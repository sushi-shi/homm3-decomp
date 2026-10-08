"""Stage the pinned GCC 2.95.2 toolchain (Debian potato i386 packages + SGI STL 3.2).

The 2000-era binaries run unmodified through the packaged ld-2.1.3.so loader:
every program the driver spawns (cpp, cc1plus, as) is reached through a
small wrapper that invokes the loader with --library-path, so no binary is
patched and no host library path leaks into the old programs.
"""
from __future__ import annotations

import gzip
import hashlib
import io
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import tomllib

from homm3.core import common


class ToolchainError(ValueError):
    pass


ROOT = common.HOMM3_DIR
DESTINATION = ROOT / "build/loki/toolchain"
SYSROOT = DESTINATION / "root"
WRAPPERS = DESTINATION / "libexec"
SGI_STL = DESTINATION / "sgi-stl"
STAMP = DESTINATION / "staged.sha256"
LOADER = SYSROOT / "lib/ld-2.1.3.so"


def specification() -> dict:
    with (ROOT / "config/loki/toolchain.toml").open("rb") as stream:
        return tomllib.load(stream)


def _gcc_lib() -> Path:
    driver = specification()["driver"]
    return SYSROOT / "usr/lib/gcc-lib" / driver["target"] / driver["version"]


def _ar_members(data: bytes):
    if not data.startswith(b"!<arch>\n"):
        raise ToolchainError("not an ar archive")
    offset = 8
    while offset + 60 <= len(data):
        header = data[offset:offset + 60]
        name = header[:16].decode().strip().rstrip("/")
        size = int(header[48:58].decode().strip())
        yield name, data[offset + 60:offset + 60 + size]
        offset += 60 + size + (size & 1)


def _extract(deb: bytes, root: Path) -> None:
    payload = next((body for name, body in _ar_members(deb) if name.startswith("data.tar")), None)
    if payload is None:
        raise ToolchainError("package has no data.tar member")
    with tarfile.open(fileobj=io.BytesIO(gzip.decompress(payload)), mode="r:") as archive:
        for member in archive.getmembers():
            relative = Path(member.name.lstrip("./"))
            if not relative.parts or ".." in relative.parts:
                continue
            path = root / relative
            if member.isdir():
                path.mkdir(parents=True, exist_ok=True)
            elif member.issym() or member.islnk():
                path.parent.mkdir(parents=True, exist_ok=True)
                target = member.linkname
                if member.islnk():
                    target = "/" + target.lstrip("./")
                if target.startswith("/"):  # keep absolute links inside the sysroot
                    target = os.path.relpath(root / target.lstrip("/"), path.parent)
                if path.is_symlink() or path.exists():
                    path.unlink()
                path.symlink_to(target)
            elif member.isfile():
                path.parent.mkdir(parents=True, exist_ok=True)
                with archive.extractfile(member) as source:
                    path.write_bytes(source.read())
                path.chmod(member.mode & 0o755 | 0o600)


def _wrapper(program: Path) -> str:
    library_path = f"{SYSROOT}/lib:{SYSROOT}/usr/lib"
    return f'#!/bin/sh\nexec "{LOADER}" --library-path "{library_path}" "{program}" "$@"\n'


def _write_wrappers() -> None:
    WRAPPERS.mkdir(parents=True, exist_ok=True)
    programs = {name: _gcc_lib() / name for name in ("cpp", "cc1", "cc1plus", "collect2")}
    programs.update({"as": SYSROOT / "usr/bin/as", "ld": SYSROOT / "usr/bin/ld"})
    for name, program in programs.items():
        if not program.is_file():
            raise ToolchainError(f"staged toolchain lacks {program}")
        path = WRAPPERS / name
        path.write_text(_wrapper(program))
        path.chmod(0o755)


def _digest(spec: dict) -> str:
    pins = {**spec["debs"], **spec["sgi_stl"]}
    return hashlib.sha256("".join(f"{k}={v}\n" for k, v in sorted(pins.items())).encode()).hexdigest()


def _read_pinned(origin: Path, pins: dict[str, str]) -> dict[str, bytes]:
    files = {}
    for name, expected in pins.items():
        try:
            data = (origin / name).read_bytes()
        except OSError as exc:
            raise ToolchainError(f"cannot read {origin / name}: {exc}") from exc
        actual = hashlib.sha256(data).hexdigest()
        if actual != expected:
            raise ToolchainError(f"{origin / name}: sha256 {actual} != pinned {expected}")
        files[name] = data
    return files


def is_staged() -> bool:
    try:
        return STAMP.read_text().strip() == _digest(specification()) and LOADER.is_file()
    except OSError:
        return False


def stage(debs: str | Path | None = None, sgi_stl: str | Path | None = None) -> Path:
    """Explicit directories > HOMM3_LOKI_DEBS/HOMM3_LOKI_SGI_STL > the staged toolchain."""
    spec = specification()
    debs = debs if debs is not None else os.environ.get("HOMM3_LOKI_DEBS")
    sgi_stl = sgi_stl if sgi_stl is not None else os.environ.get("HOMM3_LOKI_SGI_STL")
    if debs is None or sgi_stl is None:
        if is_staged():
            return DESTINATION
        raise ToolchainError("GCC 2.95.2 toolchain not staged; run `homm3 loki toolchain --debs DIR "
                             "--sgi-stl DIR` (or set HOMM3_LOKI_DEBS and HOMM3_LOKI_SGI_STL); "
                             "the pins are in config/loki/toolchain.toml")
    packages = _read_pinned(Path(debs).expanduser().resolve(), spec["debs"])
    (archive,) = _read_pinned(Path(sgi_stl).expanduser().resolve(), spec["sgi_stl"]).values()
    if DESTINATION.exists():
        shutil.rmtree(DESTINATION)
    SYSROOT.mkdir(parents=True)
    for name in sorted(packages):
        _extract(packages[name], SYSROOT)
    SGI_STL.mkdir()
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:gz") as headers:
        for member in headers.getmembers():
            if member.isfile() and "/" not in member.name.strip("./"):
                (SGI_STL / member.name.strip("./")).write_bytes(headers.extractfile(member).read())
    _write_wrappers()
    STAMP.write_text(_digest(spec) + "\n")
    return DESTINATION


def driver_command(*arguments: str) -> list[str]:
    """argv for the g++ driver; spawned tools resolve through the wrapper -B prefix.

    The 2.95 driver prepends each -B prefix, so the last one is searched first:
    the wrappers must follow the gcc-lib directory that holds specs and crt files.
    """
    gcc_lib = _gcc_lib()
    return [str(LOADER), "--library-path", f"{SYSROOT}/lib:{SYSROOT}/usr/lib",
            str(SYSROOT / "usr/bin/g++"), f"-B{gcc_lib}/", f"-B{WRAPPERS}/",
            "-nostdinc", "-nostdinc++",
            "-isystem", str(SGI_STL),
            "-isystem", str(SYSROOT / "usr/include/g++-3"),
            "-isystem", str(gcc_lib / "include"),
            "-isystem", str(SYSROOT / "usr/include"),
            *arguments]


def environment() -> dict[str, str]:
    """A clean environment: Nix COMPILER_PATH/LD_LIBRARY_PATH must not reach the driver."""
    return {"PATH": "/run/current-system/sw/bin:/usr/bin:/bin",
            "TMPDIR": os.environ.get("TMPDIR", "/tmp"), "LC_ALL": "C"}


def version() -> str:
    completed = subprocess.run(driver_command("--version"), env=environment(),
                               capture_output=True, text=True)
    if completed.returncode:
        raise ToolchainError(f"g++ 2.95.2 does not run: {completed.stderr.strip()}")
    return completed.stdout.strip()
