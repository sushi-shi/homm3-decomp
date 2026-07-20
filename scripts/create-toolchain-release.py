#!/usr/bin/env python3
"""Build and verify the HoMM3 Visual C++ 6.0 SP3 toolchain tarball.

This follows the HoMM2 two-script toolchain-release pattern.  The retail Rich
header requires the VC6 SP3 toolchain: SP3 frontends (C1/C1XX 12.00.8472), the
SP3 backend (C2 12.00.8447), LINK 6.00.8447, and CVTRES 5.00.1736.1. Those
files come from Microsoft's complete ten-part SP3 Internet distribution
preserved on the pinned TechNet disc.
The September 1999 DirectX 7 SDK supplies the DirectDraw headers and libraries:
its DirectDrawCreate hint 8 matches the retail image, whereas VC6/DX5 and the
DX6.0/6.1 SDK libraries use hint 7. RTM binaries/libraries are retained for FID
and mixed-build calibration.
"""
from __future__ import annotations

import hashlib
import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(os.environ.get("HOMM3_DIR", Path(__file__).resolve().parent.parent)).resolve()
RELEASE_EPOCH = 1784419200
BIN_VERSIONS = {
    "c1.dll": "12.00.8472.0",
    "c1xx.dll": "12.00.8472.0",
    "c2.dll": "12.00.8447.0",
    "link.exe": "6.00.8447.0",
    "cvtres.exe": "5.00.1736.1",
}
REQUIRED_LIBS = ("libc.lib", "libcmt.lib", "libcp.lib", "libcpmt.lib",
                 "oldnames.lib", "uuid.lib")
SP3_LIBS = ("libc.lib", "libcmt.lib", "uuid.lib")
COMMON_BIN_FILES = ("mspdb60.dll", "msobj10.dll", "rc.exe", "rcdll.dll")
REQUIRED_HEADERS = ("algorithm", "exception", "functional", "stdexcept",
                    "streambuf", "strstream", "vector", "windows.h",
                    "ddraw.h")
DIRECTX7_SHA1 = {
    "ddraw.h": "1deffbcb2f7e7de1e951f709b3925b2bd85a436d",
    "ddraw.lib": "6b10bd77b66a603e4688738720b13ed552cdfa8f",
}


def log(message: str) -> None:
    print("[toolchain] " + message, flush=True)


def extract(source: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    subprocess.run(["7z", "x", "-y", "-o" + str(destination), str(source)],
                   check=True, stdout=subprocess.DEVNULL)


def extract_members(source: Path, destination: Path, *members: str) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    subprocess.run(["7z", "x", "-y", "-o" + str(destination), str(source),
                    *members], check=True, stdout=subprocess.DEVNULL)


def find_dir(root: Path, name: str) -> Path:
    matches = [path for path in root.rglob("*")
               if path.is_dir() and path.name.lower() == name.lower()]
    if not matches:
        raise SystemExit("could not find %s under %s" % (name, root))
    return min(matches, key=lambda path: len(path.parts))


def find_file(root: Path, name: str) -> Path:
    matches = [path for path in root.rglob("*")
               if path.is_file() and path.name.lower() == name.lower()]
    if not matches:
        raise SystemExit("could not find %s under %s" % (name, root))
    return min(matches, key=lambda path: (len(path.parts), path.stat().st_size))


def replace_file_ci(source: Path, destination_dir: Path, name: str) -> Path:
    """Replace an installed file without creating a case-only duplicate."""
    destination = find_file(destination_dir, name)
    shutil.copy2(source, destination)
    return destination


def overlay_files_ci(source_dir: Path, destination_dir: Path) -> int:
    """Install one SDK directory without case-only duplicate filenames."""
    installed = 0
    for source in sorted(source_dir.iterdir(), key=lambda path: path.name.lower()):
        if not source.is_file():
            continue
        matches = [path for path in destination_dir.iterdir()
                   if path.is_file() and path.name.lower() == source.name.lower()]
        if len(matches) > 1:
            raise SystemExit("case-ambiguous SDK destination: " + source.name)
        destination = matches[0] if matches else destination_dir / source.name
        shutil.copy2(source, destination)
        installed += 1
    return installed


def apply_include_installer_aliases(base_tree: Path, include_dir: Path) -> int:
    """Apply the VC98 include renames encoded by Microsoft's installer.

    The retail CD uses ISO-9660-compatible source names such as XCEPTION and
    ALGRITHM. VS98ENT.INF records the installed names after ``<``. Copying the
    VC98 tree without applying this table leaves a compiler that cannot include
    several standard C++ and Platform SDK headers.
    """
    setup_inf = find_file(base_tree, "VS98ENT.INF")
    text = setup_inf.read_text(encoding="latin-1")
    aliases = sorted(set(re.findall(
        r"vc98\\include\\([^,<\r\n]+)<([^>\r\n]+)>", text,
        flags=re.IGNORECASE)))
    if not aliases:
        raise SystemExit("VS98ENT.INF contains no VC98 include aliases")

    installed = 0
    for source_name, installed_name in aliases:
        sources = [path for path in include_dir.iterdir()
                   if path.is_file() and path.name.lower() == source_name.lower()]
        if len(sources) != 1:
            raise SystemExit("installer alias source %s has %d matches" %
                             (source_name, len(sources)))
        targets = [path for path in include_dir.iterdir()
                   if path.is_file() and path.name.lower() == installed_name.lower()]
        if targets and targets != sources:
            raise SystemExit("installer alias target already exists: %s" %
                             installed_name)
        sources[0].rename(include_dir / installed_name)
        installed += 1
    return installed


def pe_file_version(path: Path) -> str:
    """Read the fixed file-version text stored in a VC6 PE VERSIONINFO."""
    text = path.read_bytes().decode("utf-16le", errors="ignore")
    match = re.search(r"FileVersion\x00+([0-9]+(?:\.[0-9]+){3})", text)
    if not match:
        raise SystemExit("could not read FileVersion from %s" % path)
    return match.group(1)


def verify_version(path: Path, expected: str) -> None:
    actual = pe_file_version(path)
    if actual != expected:
        raise SystemExit("%s has FileVersion %s, expected %s" %
                         (path, actual, expected))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sha1(path: Path) -> str:
    digest = hashlib.sha1()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> None:
    base_value = os.environ.get("VC6_DISC1")
    sp3_value = os.environ.get("VC6_SP3")
    directx_value = os.environ.get("DIRECTX7_SDK_MEDIA")
    if not base_value:
        raise SystemExit("VC6_DISC1 is unset; use create-toolchain-release.nix")
    if not sp3_value:
        raise SystemExit("VC6_SP3 must point to Microsoft's complete SP3 media")
    if not directx_value:
        raise SystemExit("DIRECTX7_SDK_MEDIA must point to the original DirectX 7 SDK ISO")

    base = Path(base_value).resolve()
    sp3 = Path(sp3_value).resolve()
    directx = Path(directx_value).resolve()
    if not all(path.is_file() for path in (base, sp3, directx)):
        raise SystemExit("toolchain media path is not a file")

    output = Path(os.environ.get(
        "OUTPUT", ROOT / "build/homm3-toolchain-vc6-sp3.tar.xz")).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix=".vc6-sp3-", dir=output.parent))
    try:
        base_tree = work / "base"
        sp3_tree, sp3_payload = work / "sp3", work / "sp3-payload"
        sp3_cabs, sp3_update = work / "sp3-cabs", work / "sp3-update"
        directx_tree = work / "directx7"
        stage, msvc = work / "stage", work / "stage/msvc"

        log("extracting pinned Visual Studio 6.0 Enterprise Disc 1")
        extract(base, base_tree)
        vc98 = find_dir(base_tree, "VC98")
        for name in ("BIN", "INCLUDE", "LIB", "MFC"):
            source = next((path for path in vc98.iterdir()
                           if path.name.lower() == name.lower()), None)
            if source:
                shutil.copytree(source, msvc / name.lower())
        if not (msvc / "bin").is_dir() or not (msvc / "include").is_dir():
            raise SystemExit("Disc 1 did not yield a complete VC98 tree")
        alias_count = apply_include_installer_aliases(base_tree,
                                                      msvc / "include")
        log("applied %d VC98 include aliases from VS98ENT.INF" % alias_count)
        for name in REQUIRED_HEADERS:
            find_file(msvc / "include", name)
        # A normal VC6 installation adds COMMON/MSDEV98/BIN to PATH. Put the
        # command-line compiler's required PDB/resource helpers beside CL so
        # the bundle is relocatable and does not depend on an IDE install.
        common = find_dir(base_tree / "COMMON", "MSDEV98")
        common_bin = find_dir(common, "BIN")
        for name in COMMON_BIN_FILES:
            shutil.copy2(find_file(common_bin, name), msvc / "bin" / name.upper())
        shutil.copytree(msvc / "lib", msvc / "lib-rtm")

        log("extracting complete Visual Studio 6.0 SP3 Internet payload")
        sp3_parts = tuple("VSTUDIO/SP3/VS6SP3_%d.EXE" % index
                          for index in range(1, 11))
        extract_members(sp3, sp3_tree, *sp3_parts)

        # The first self-extractor carries setup and the C2 backend directly.
        # The remaining nine self-extractors each carry one volume of the
        # multi-volume cabinet containing the rest of the SP3 update.
        sp3_sfx = find_file(sp3_tree, "VS6SP3_1.EXE")
        extract(sp3_sfx, sp3_payload)
        for index in range(2, 11):
            extract(find_file(sp3_tree, "VS6SP3_%d.EXE" % index), sp3_cabs)
        for index in range(1, 10):
            source = find_file(sp3_cabs, "VS6sp3_%d.cab" % index)
            destination = sp3_cabs / ("Vs6sp3_%d.cab" % index)
            source.rename(destination)

        extract_members(
            sp3_cabs / "Vs6sp3_1.cab", sp3_update,
            "vc98/bin/c1.dll", "vc98/bin/c1xx.dll",
            "vc98/bin/link.exe", "vc98/bin/cvtres.exe",
            "vc98/lib/libc.lib", "vc98/lib/libcmt.lib",
            "vc98/lib/uuid.Lib")
        sp3_vc98 = find_dir(sp3_update, "vc98")
        sp3_bin = find_dir(sp3_vc98, "bin")
        sp3_lib = find_dir(sp3_vc98, "lib")

        for name in ("c1.dll", "c1xx.dll", "link.exe", "cvtres.exe"):
            source = find_file(sp3_bin, name)
            verify_version(source, BIN_VERSIONS[name])
            replace_file_ci(source, msvc / "bin", name)
            log("verified %s FileVersion %s" % (name, BIN_VERSIONS[name]))
        c2 = find_file(sp3_payload, "msvcep.dll")
        verify_version(c2, BIN_VERSIONS["c2.dll"])
        replace_file_ci(c2, msvc / "bin", "c2.dll")
        log("verified c2.dll FileVersion %s" % BIN_VERSIONS["c2.dll"])
        # SP3's published VC++ file manifest explicitly replaces these three.
        # Other required archives remain the verified RTM copies unless later
        # FID calibration proves a distinct SP3 body set is required.
        for name in SP3_LIBS:
            source = find_file(sp3_lib, name)
            replace_file_ci(source, msvc / "lib", name)
        for name in REQUIRED_LIBS:
            try:
                find_file(msvc / "lib-rtm", name)
            except SystemExit:
                raise SystemExit("RTM library missing: " + name)
            try:
                find_file(msvc / "lib", name)
            except SystemExit:
                raise SystemExit("SP3 library missing: " + name)

        log("extracting and applying pinned Microsoft DirectX 7 SDK")
        extract_members(directx, directx_tree, "DXF/include/*", "DXF/lib/*")
        directx_include = directx_tree / "DXF/include"
        directx_lib = directx_tree / "DXF/lib"
        for name, expected in DIRECTX7_SHA1.items():
            root = directx_include if name.endswith(".h") else directx_lib
            source = find_file(root, name)
            actual = sha1(source)
            if actual != expected:
                raise SystemExit("DirectX 7 %s has SHA1 %s, expected %s" %
                                 (name, actual, expected))
        header_count = overlay_files_ci(directx_include, msvc / "include")
        library_count = overlay_files_ci(directx_lib, msvc / "lib")
        log("installed %d DirectX 7 headers and %d libraries" %
            (header_count, library_count))

        manifest = stage / "TOOLCHAIN.txt"
        manifest.write_text(
            "HoMM3 matching toolchain: Visual C++ 6.0 SP3\n"
            "C1/C1XX 12.00.8472; C2/LINK 12.00.8447; "
            "CVTRES 5.00.1736.1\n"
            "lib-rtm preserves build-8168 libraries for FID calibration.\n"
            "VC98 include aliases are installed from Microsoft's VS98ENT.INF.\n"
            "DirectX 7 SDK (September 1999) overlays VC6's DirectX 5 files.\n"
            "DirectX 7 ddraw.lib SHA1: 6b10bd77b66a603e4688738720b13ed552cdfa8f.\n"
            "target Rich fingerprint: C++ 8447 + RTM 8168 library objects.\n")
        log("packaging " + str(output))
        subprocess.run([
            "tar", "--sort=name", "--format=gnu", "--owner=0", "--group=0",
            "--numeric-owner", "--mtime=@%d" % RELEASE_EPOCH,
            "--transform", "s|^\\.|homm3-toolchain-vc6-sp3|", "-C",
            str(stage), "-cJf", str(output), ".",
        ], check=True)
        print("Output: " + str(output))
        print("SHA256: " + sha256(output))
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
