"""Link the Loki h3maped image from the 103 project objects, the link-only
version object and the staged libraries.

    build/h3maped-loki/link/h3maped        the linked, stripped executable
    build/h3maped-loki/link/h3maped.map    ld's link map (input sections, archive members)
    build/h3maped-loki/link/link.log       the ld command and its output

The image's own facts fix the command (docs/loki/README.md, "Link"):

- link order: its .comment entries and .eh_frame CIEs run crt1.o, crti.o (egcs
  1.1.2, glibc 2.1.3), crtbegin.o, the project objects in census order, the
  version object (game_version; GCC 2.95.2 C, no .text, a CIE without FDEs), libglade,
  libxml, GTK+, GDK, GModule, GLib, zlib, libstdc++, libgcc, then crtend.o, crtn.o;
- DT_NEEDED is libdl, libXi, libXext, libX11, libm, libc, in that order (libm first
  from g++'s `-lstdc++ -lm` tail: an earlier -lm versions libstdc++'s clog), so those
  six are the only shared libraries; everything else is static;
- `--export-dynamic` (libglade connects signal handlers by name through .dynsym),
  the `/lib/ld-linux.so.2` interpreter, and no .symtab (`-s`);
- binutils 2.9.1's ld (orphan .gcc_except_table before .eh_frame, after .data).
"""
from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import re
import subprocess

from homm3.loki import build, toolchain

OUT = build.OUT / "link"
IMAGE = OUT / "h3maped"
MAP = OUT / "h3maped.map"
LOG = OUT / "link.log"

# Static archives after the project objects, in the image's member order.
ARCHIVES_BEFORE = ("libglade.a", "libxml.a", "libgtk.a", "libgdk.a", "libgmodule.a", "libglib.a")
SHARED = ("-ldl", "-lXi", "-lXext", "-lX11")
UNDEFINED = re.compile(r"undefined reference to `([^']+)'")


def project_objects() -> list[Path]:
    """The compiled units in link order (their census object index); a link-only
    object follows the census object it names."""
    order = [(unit.obj, 0, unit.name) for unit in build.units()]
    order += [(after, 1, unit.name) for unit, after in build.link_only_units()]
    return [build.OUT / "obj" / f"{name}.o" for _, _, name in sorted(order)]


def command(output: Path = IMAGE, map_path: Path = MAP) -> list[str]:
    lib = toolchain.LINK / "lib"
    sysroot = toolchain.SYSROOT
    library_dirs = [toolchain.LINK / "glibc", lib, toolchain.LINK / "xlib", sysroot / "usr/lib", sysroot / "lib"]
    return [str(toolchain.LINK / "libexec/ld"), "-m", "elf_i386",
            "-export-dynamic", "-dynamic-linker", "/lib/ld-linux.so.2", "-s",
            "-o", str(output), "-Map", str(map_path),
            "-rpath-link", ":".join(str(d) for d in (library_dirs[0], *library_dirs[2:])),
            *(f"-L{d}" for d in library_dirs),
            str(lib / "crt1.o"), str(lib / "crti.o"), str(lib / "crtbegin.o"),
            *(str(o) for o in project_objects()),
            *(str(lib / a) for a in ARCHIVES_BEFORE),
            *SHARED,
            str(lib / "libz.a"),
            # g++'s own tail: -lstdc++ -lm, then -lgcc -lc -lgcc and the end files.
            str(lib / "libstdc++.a"), "-lm", str(lib / "libgcc.a"), "-lc", str(lib / "libgcc.a"),
            str(lib / "crtend.o"), str(lib / "crtn.o")]


def compile_all(jobs: int) -> list[str]:
    failures = []
    units = build.units() + [unit for unit, _ in build.link_only_units()]
    with ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
        for unit, (_, error) in zip(units, pool.map(build.compile_unit, units)):
            if error:
                failures.append(f"{unit.name}: {error}")
    return failures


def link(jobs: int = 3, compile_units: bool = True) -> tuple[int, list[str]]:
    """Link the image; returns ld's exit status and the undefined symbols it reported."""
    toolchain.stage()
    toolchain.stage_libraries()
    if compile_units:
        failures = compile_all(jobs)
        if failures:
            raise RuntimeError("compile failed: " + "; ".join(failures))
    missing = [str(o) for o in project_objects() if not o.is_file()]
    if missing:
        raise RuntimeError(f"missing objects (run `homm3 loki build`): {', '.join(missing[:5])}")
    OUT.mkdir(parents=True, exist_ok=True)
    IMAGE.unlink(missing_ok=True)
    argv = command()
    completed = subprocess.run(argv, env=toolchain.environment(), capture_output=True, text=True)
    LOG.write_text("$ " + " ".join(argv) + "\n" + completed.stdout + completed.stderr)
    undefined = sorted(set(UNDEFINED.findall(completed.stdout + completed.stderr)))
    return completed.returncode, undefined


def main(jobs: int = 3, compile_units: bool = True) -> int:
    status, undefined = link(jobs, compile_units)
    if undefined:
        print(f"[loki] link: {len(undefined)} undefined symbols:")
        for name in undefined:
            print(f"         {name}")
    if status:
        print(f"[loki] link failed (exit {status}); see {LOG.relative_to(build.ROOT)}")
        return 1
    print(f"[loki] linked {IMAGE.relative_to(build.ROOT)} ({IMAGE.stat().st_size} bytes); "
          f"map {MAP.relative_to(build.ROOT)}")
    from homm3.loki import linkdiff
    ok, lines = linkdiff.check(IMAGE)
    print("\n".join(lines))
    return 0 if ok else 1
