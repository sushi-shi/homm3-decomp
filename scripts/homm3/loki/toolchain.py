"""Stage the pinned GCC 2.95.2 toolchain: Debian potato i386 packages (driver,
cpp, libc, libstdc++), the vanilla 2.95.2 release compilers (cc1, cc1plus) and
binutils 2.9.1.0.25 `as` of Slackware 7.1, SGI STL 3.2, and the GTK+/GLib 1.2.8
headers of Slackware 7.1.

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
BINUTILS = DESTINATION / "binutils"
COMPILERS = DESTINATION / "gcc-2.95.2-release"
GTK = DESTINATION / "gtk"
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


def _wrapper(program: Path, *library_dirs: Path) -> str:
    library_path = ":".join(str(p) for p in (SYSROOT / "lib", SYSROOT / "usr/lib", *library_dirs))
    return f'#!/bin/sh\nexec "{LOADER}" --library-path "{library_path}" "{program}" "$@"\n'


def _write_wrappers() -> None:
    WRAPPERS.mkdir(parents=True, exist_ok=True)
    programs = {name: _gcc_lib() / name for name in ("cpp", "collect2")}
    programs.update({name: COMPILERS / name for name in ("cc1", "cc1plus")})
    programs.update({"as": BINUTILS / "usr/bin/as", "ld": SYSROOT / "usr/bin/ld"})
    for name, program in programs.items():
        if not program.is_file():
            raise ToolchainError(f"staged toolchain lacks {program}")
        path = WRAPPERS / name
        path.write_text(_wrapper(program, BINUTILS / "usr/lib") if name == "as" else _wrapper(program))
        path.chmod(0o755)


def _digest(spec: dict) -> str:
    pins = {**spec["debs"], **spec["sgi_stl"], **spec["binutils"], **spec["compilers"], **spec["gtk"]}
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


def stage(debs: str | Path | None = None, sgi_stl: str | Path | None = None,
          binutils: str | Path | None = None, compilers: str | Path | None = None,
          gtk: str | Path | None = None) -> Path:
    """Explicit directories > HOMM3_LOKI_DEBS/_SGI_STL/_BINUTILS/_GCC/_GTK > the staged toolchain."""
    spec = specification()
    debs = debs if debs is not None else os.environ.get("HOMM3_LOKI_DEBS")
    sgi_stl = sgi_stl if sgi_stl is not None else os.environ.get("HOMM3_LOKI_SGI_STL")
    binutils = binutils if binutils is not None else os.environ.get("HOMM3_LOKI_BINUTILS")
    compilers = compilers if compilers is not None else os.environ.get("HOMM3_LOKI_GCC")
    gtk = gtk if gtk is not None else os.environ.get("HOMM3_LOKI_GTK")
    if debs is None or sgi_stl is None or binutils is None or compilers is None or gtk is None:
        if is_staged():
            return DESTINATION
        raise ToolchainError("GCC 2.95.2 toolchain not staged; run `homm3 loki toolchain --debs DIR "
                             "--sgi-stl DIR --binutils DIR --gcc DIR --gtk DIR` (or set HOMM3_LOKI_DEBS, "
                             "HOMM3_LOKI_SGI_STL, HOMM3_LOKI_BINUTILS, HOMM3_LOKI_GCC and HOMM3_LOKI_GTK); "
                             "the pins are in config/loki/toolchain.toml")
    packages = _read_pinned(Path(debs).expanduser().resolve(), spec["debs"])
    (archive,) = _read_pinned(Path(sgi_stl).expanduser().resolve(), spec["sgi_stl"]).values()
    (assembler,) = _read_pinned(Path(binutils).expanduser().resolve(), spec["binutils"]).values()
    (release,) = _read_pinned(Path(compilers).expanduser().resolve(), spec["compilers"]).values()
    toolkit_files = _read_pinned(Path(gtk).expanduser().resolve(), spec["gtk"])
    toolkit = toolkit_files["gtkglib.tgz"]
    glade = toolkit_files["libglade-0.14.tar.gz"]
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
    # Slackware's package: only the assembler and the libbfd it loads.
    with tarfile.open(fileobj=io.BytesIO(assembler), mode="r:gz") as package:
        for member in package.getmembers():
            name = member.name.lstrip("./")
            if member.isfile() and (name == "usr/bin/as" or name.startswith("usr/lib/libbfd-")):
                path = BINUTILS / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(package.extractfile(member).read())
                path.chmod(0o755)
    # Slackware's vanilla 2.95.2: only the compilers proper. Its cccp rejects
    # the Windows SDK headers the shared source once imported; potato's cpp stays.
    prefix = "usr/lib/gcc-lib/i386-slackware-linux/2.95.2/"
    with tarfile.open(fileobj=io.BytesIO(release), mode="r:gz") as package:
        for member in package.getmembers():
            name = member.name.lstrip("./")
            if member.isfile() and name in {prefix + p for p in ("cc1", "cc1plus")}:
                path = COMPILERS / name[len(prefix):]
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(package.extractfile(member).read())
                path.chmod(0o755)
    # Slackware's GTK+/GLib 1.2.8 package: only the headers (`gtk-config --cflags`).
    with tarfile.open(fileobj=io.BytesIO(toolkit), mode="r:gz") as package:
        for member in package.getmembers():
            name = member.name.lstrip("./")
            if member.isfile() and (name in ("usr/include/glib.h", "usr/include/gmodule.h",
                                             "usr/lib/glib/include/glibconfig.h")
                                    or name.startswith(("usr/include/gdk/", "usr/include/gtk/"))):
                path = GTK / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(package.extractfile(member).read())
    # libglade 0.14's public headers, from its source release (glade-config --cflags).
    with tarfile.open(fileobj=io.BytesIO(glade), mode="r:gz") as package:
        for member in package.getmembers():
            name = Path(member.name)
            if member.isfile() and name.parent.name == "glade" and name.name in (
                    "glade.h", "glade-xml.h", "glade-build.h", "glade-widget-tree.h"):
                path = GTK / "usr/include/glade" / name.name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(package.extractfile(member).read())
    _write_wrappers()
    STAMP.write_text(_digest(spec) + "\n")
    return DESTINATION


LINK = DESTINATION / "link"
LINK_STAMP = LINK / "staged.sha256"
LINK_CFLAGS = "-O2 -mcpu=pentiumpro"  # Loki's i686-configured 2.95.2 at -O2 (libglade, libxml)
# Recipe version: bump when what stage_libraries() builds changes for the same media.
LINK_RECIPE = "1"


def _link_digest(spec: dict) -> str:
    pins = "".join(f"{k}={v}\n" for k, v in sorted(spec["link"].items()))
    return hashlib.sha256(f"{_digest(spec)}\n{pins}recipe={LINK_RECIPE}\n".encode()).hexdigest()


def link_staged() -> bool:
    try:
        return is_staged() and LINK_STAMP.read_text().strip() == _link_digest(specification())
    except OSError:
        return False


def _unpack(archive: bytes, members: dict[str, Path]) -> None:
    """Extract named members of a .tgz to explicit destinations; every one must exist."""
    found = set()
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:gz") as package:
        for member in package.getmembers():
            name = member.name.lstrip("./")
            if member.isfile() and name in members:
                path = members[name]
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(package.extractfile(member).read())
                path.chmod(member.mode & 0o755 | 0o600)
                found.add(name)
    missing = sorted(set(members) - found)
    if missing:
        raise ToolchainError(f"package lacks {', '.join(missing)}")


def _unpack_source(archive: bytes, destination: Path) -> Path:
    """Extract a source release below destination; returns its top directory."""
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:gz") as package:
        top = package.getnames()[0].lstrip("./").split("/")[0]
        package.extractall(destination, filter="data")
    return destination / top


def _script(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    path.chmod(0o755)


def _build_compiler() -> str:
    """A `cc` for the libraries' configure and make: the release cc1 through the
    potato driver, able to link and run configure's test programs through the
    staged loader and libraries (link flags never reach the kept objects)."""
    gcc_lib = _gcc_lib()
    run_path = f"{SYSROOT}/lib:{SYSROOT}/usr/lib:{LINK}/xlib"
    return (
        "#!/bin/sh\n"
        f'LINKFLAGS="-Wl,-dynamic-linker,{LOADER} -Wl,-rpath,{run_path} -Wl,-rpath-link,{run_path}"\n'
        'for a in "$@"; do case "$a" in -c|-S|-E|-M|-MM) LINKFLAGS=;; esac; done\n'
        f'exec env -i PATH={WRAPPERS}:/run/current-system/sw/bin:/usr/bin:/bin TMPDIR="${{TMPDIR:-/tmp}}" '
        f'"{LOADER}" --library-path "{SYSROOT}/lib:{SYSROOT}/usr/lib" "{SYSROOT}/usr/bin/gcc" '
        f"-B{gcc_lib}/ -B{SYSROOT}/usr/lib/ -B{LINK}/libexec/ -B{WRAPPERS}/ -nostdinc "
        f"-isystem {gcc_lib}/include -isystem {SYSROOT}/usr/include "
        f'-L{LINK}/lib -L{LINK}/xlib -L{SYSROOT}/usr/lib -L{SYSROOT}/lib $LINKFLAGS "$@"\n')


def _run_build(command: list[str], cwd: Path, env: dict[str, str], log: Path) -> None:
    with log.open("a") as stream:
        stream.write("$ " + " ".join(command) + "\n")
        stream.flush()
        completed = subprocess.run(command, cwd=cwd, env=env, stdout=stream, stderr=subprocess.STDOUT)
    if completed.returncode:
        raise ToolchainError(f"{' '.join(command[:2])} failed in {cwd} (see {log})")


def stage_libraries(libs: str | Path | None = None, jobs: int = 2) -> Path:
    """Stage the link media of config/loki/toolchain.toml [link] under build/loki/toolchain/link
    and build libxml 1.8.9 and libglade 0.14 from source with the release compiler."""
    spec = specification()
    libs = libs if libs is not None else os.environ.get("HOMM3_LOKI_LIBS")
    if libs is None:
        if link_staged():
            return LINK
        raise ToolchainError("Loki link media not staged; run `homm3 loki toolchain --libs DIR` "
                             "(or set HOMM3_LOKI_LIBS); the pins are config/loki/toolchain.toml [link]")
    stage()
    media = _read_pinned(Path(libs).expanduser().resolve(), spec["link"])
    if LINK.exists():
        shutil.rmtree(LINK)
    lib, xlib = LINK / "lib", LINK / "xlib"
    bfd = "usr/lib/libbfd-2.9.1.0.25.so"
    _unpack(media["binutils.tgz"], {"usr/bin/ld": LINK / "binutils/usr/bin/ld", bfd: LINK / "binutils" / bfd})
    _script(LINK / "libexec/ld", _wrapper(LINK / "binutils/usr/bin/ld", LINK / "binutils/usr/lib"))
    _unpack(media["glibc.tgz"], {f"usr/lib/{name}": lib / name
                                 for name in ("crt1.o", "crti.o", "crtn.o", "libc_nonshared.a")})
    # libc.so is a linker script naming /lib/libc.so.6; ld 2.9.1 has no --sysroot.
    (lib / "libc.so").write_text(f"/* GNU ld script (staged) */\n"
                                 f"GROUP ( {SYSROOT}/lib/libc.so.6 {lib}/libc_nonshared.a )\n")
    x_libraries = {"libX11.so.6.1": "libX11", "libXext.so.6.3": "libXext", "libXi.so.6.0": "libXi"}
    _unpack(media["xbin.tgz"], {f"usr/X11R6/lib/{name}": xlib / name for name in x_libraries})
    for name, stem in x_libraries.items():
        for alias in (f"{stem}.so.6", f"{stem}.so"):
            (xlib / alias).symlink_to(name)
    _unpack(media["gtkglib.tgz"], {f"usr/lib/{name}": lib / name
                                   for name in ("libgtk.a", "libgdk.a", "libgmodule.a", "libglib.a")})
    _unpack(media["zlib.tgz"], {"usr/lib/libz.a": lib / "libz.a",
                                "usr/include/zlib.h": LINK / "include/zlib.h",
                                "usr/include/zconf.h": LINK / "include/zconf.h"})
    prefix = "usr/lib/gcc-lib/i386-slackware-linux/2.95.2/"
    _unpack(media["gcc.tgz"], {prefix + "crtbegin.o": lib / "crtbegin.o", prefix + "crtend.o": lib / "crtend.o",
                               prefix + "libgcc.a": lib / "libgcc.a",
                               "usr/lib/libstdc++-3-libc6.1-2-2.10.0.a": lib / "libstdc++.a"})
    # libxml, then libglade against it: configure and make, as Loki built them.
    bin_dir = LINK / "bin"
    _script(bin_dir / "cc", _build_compiler())
    gtk_cflags = f"-I{GTK}/usr/include -I{GTK}/usr/lib/glib/include"
    gtk_libs = f"-L{lib} -L{xlib} -lgtk -lgdk -rdynamic -lgmodule -lglib -ldl -lXi -lXext -lX11 -lm"
    for name, libraries in (("glib-config", f"-L{lib} -rdynamic -lgmodule -lglib -ldl"), ("gtk-config", gtk_libs)):
        _script(bin_dir / name, "#!/bin/sh\nfor a in \"$@\"; do case $a in --version) echo 1.2.8;; "
                f"--cflags) echo \"{gtk_cflags}\";; --libs) echo \"{libraries}\";; esac; done\n")
    env = {**os.environ, "PATH": f"{bin_dir}:{os.environ.get('PATH', '/usr/bin:/bin')}", "CC": str(bin_dir / "cc"),
           "CFLAGS": LINK_CFLAGS, "CPPFLAGS": f"-I{LINK}/include", "LDFLAGS": f"-L{lib}", "LC_ALL": "C"}
    env.pop("CONFIG_SITE", None)
    log = LINK / "build.log"
    configure = ["./configure", f"--prefix={LINK}", "--disable-shared", "--enable-static", "i686-pc-linux-gnu"]
    xml = _unpack_source(media["libxml-1.8.9.tar.gz"], LINK / "src")
    _run_build(configure, xml, env, log)
    _run_build(["make", f"-j{jobs}", "libxml.la", "xml-config"], xml, env, log)
    _run_build(["make", "install-libLTLIBRARIES", "install-xmlincHEADERS", "install-binSCRIPTS"], xml, env, log)
    glade = _unpack_source(media["libglade-0.14.tar.gz"], LINK / "src")
    _run_build([*configure, "--without-gnome", "--disable-bonobo"], glade, env, log)
    _run_build(["make", f"-j{jobs}", "libglade.la"], glade / "glade", env, log)
    shutil.copyfile(glade / "glade/.libs/libglade.a", lib / "libglade.a")
    LINK_STAMP.write_text(_link_digest(spec) + "\n")
    return LINK


def driver_command(*arguments: str, driver: str = "g++") -> list[str]:
    """argv for the g++ (or gcc) driver; spawned tools resolve through the wrapper -B prefix.

    The g++ driver compiles a .c file as C++; Loki's C objects (Glade's
    main.c and support.c) need the gcc driver, which runs cc1.

    The 2.95 driver prepends each -B prefix, so the last one is searched first:
    the wrappers must follow the gcc-lib directory that holds specs and crt files.
    """
    gcc_lib = _gcc_lib()
    return [str(LOADER), "--library-path", f"{SYSROOT}/lib:{SYSROOT}/usr/lib",
            str(SYSROOT / "usr/bin" / driver), f"-B{gcc_lib}/", f"-B{WRAPPERS}/",
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
