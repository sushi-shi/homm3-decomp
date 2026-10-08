"""Build the libraries h3maped links statically, as the image shows they were built.

Called by `homm3 loki toolchain --libs DIR` (toolchain.stage_libraries) with the
media pinned in config/loki/toolchain.toml [link]; everything lands in
build/loki/toolchain/link/:

    lib/      crt1.o crti.o crtn.o libc_nonshared.a libc.so (glibc 2.1.3, egcs)
              crtbegin.o crtend.o libgcc.a libstdc++.a       (GCC 2.95.2, i686)
              libglade.a libxml.a                             (GCC 2.95.2 -O2)
              libgtk.a libgdk.a libgmodule.a libglib.a libz.a (egcs 1.1.2)
    xlib/     libX11.so.6, libXi.so.6 (Red Hat 6.2), libXext.so.6 (Red Hat 7.0)
    glibc/    libc.so.6, libm.so.6, libdl.so.2 (Red Hat 6.0 glibc 2.1.1-6), for h3maped's link
    libexec/  ld (binutils 2.9.1.0.25)
    build.log every configure and make

Two compilers build them, both 2000-era binaries run through potato's loader:

- GCC 2.95.2: the release (the compile toolchain's cc1/cc1plus), with Loki's
  i686 default `-mcpu=pentiumpro`, builds libxml 1.8.9 and libglade 0.14 at
  `-O2` against Red Hat 6.0's glibc 2.1.1-6 headers; the stage-1 cc1/cc1plus of
  an i686-pc-linux-gnu 2.95.2 tree, compiled by egcs 1.1.2 at configure's
  default `-g -O2` (a plain `make`, no bootstrap), builds libgcc, crtstuff and
  libio/libstdc++ 2.10 the way that tree builds its target libraries
  (`-g -O2`). 2.95.2's build_x_typeid reads an uninitialized `nonnull`
  (fixed_type_or_null never sets it for `*this`), so whether `typeid(*this)`
  tests `this` depends on the stack the compiling cc1plus left: this one omits
  the test in libgcc's `exception::what()`, as the image does; a bootstrapped
  (stage 3) or release cc1plus emits it;
- egcs 1.1.2 of Red Hat 6.2 (egcs-1.1.2-30, its cpp and cc1, binutils-2.9.5.0.22-6's
  as): GLib/GTK+ 1.2.8 (with Red Hat's ahiguti i18n patch) at
  Red Hat's `-O2 -m486 -fno-strength-reduce`, zlib 1.0.8 at
  `-O2 -fno-strength-reduce`. It emits `.p2align 4,,7` for jump targets, as the
  image's C libraries show (Slackware's egcs emits `.align 16`).
"""
from __future__ import annotations

import gzip
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess

from homm3.loki import toolchain as tc

EGCS_LIB = "usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66"
GCC_CFLAGS_I686 = "-mcpu=pentiumpro"  # TARGET_CPU_DEFAULT of an i686-pc-linux-gnu 2.95.2
XML_GLADE_CFLAGS = "-O2"
REDHAT_CFLAGS = "-O2 -m486 -fno-strength-reduce"  # Red Hat 6.2 RPM_OPT_FLAGS (i386)
ZLIB_CFLAGS = "-O2 -fno-strength-reduce"
X_LIBRARIES = {"libX11.so.6.1": "libX11", "libXext.so.6.4": "libXext", "libXi.so.6.0": "libXi"}


class BuildError(tc.ToolchainError):
    pass


def _rpm_header_size(data: bytes, offset: int) -> int:
    if data[offset:offset + 3] != b"\x8e\xad\xe8":
        raise BuildError("bad RPM header")
    count, size = struct.unpack_from(">II", data, offset + 8)
    return 16 + 16 * count + size


def rpm_files(data: bytes) -> dict[str, tuple[int, bytes]]:
    """name -> (mode, content) of a v3 RPM's regular files and symlinks (cpio newc payload)."""
    if data[:4] != b"\xed\xab\xee\xdb":
        raise BuildError("not an RPM")
    offset = 96
    offset += _rpm_header_size(data, offset)
    offset = (offset + 7) & ~7
    offset += _rpm_header_size(data, offset)
    payload = gzip.decompress(data[offset:])
    files, inodes, pos = {}, {}, 0
    while pos + 110 <= len(payload):
        if payload[pos:pos + 6] != b"070701":
            raise BuildError("bad cpio header")
        fields = [int(payload[pos + 6 + 8 * i:pos + 14 + 8 * i], 16) for i in range(13)]
        inode, mode, size, namesize = fields[0], fields[1], fields[6], fields[11]
        name_start = pos + 110
        name = payload[name_start:name_start + namesize - 1].decode()
        body_start = (name_start + namesize + 3) & ~3
        body = payload[body_start:body_start + size]
        pos = (body_start + size + 3) & ~3
        if name == "TRAILER!!!":
            break
        if mode & 0o170000 in (0o100000, 0o120000):
            files[name.lstrip("./")] = (mode, body)
            inodes.setdefault(inode, []).append(name.lstrip("./"))
    # Hard links: cpio stores the data once, with the last name of the inode.
    for names in inodes.values():
        content = next((files[n][1] for n in names if files[n][1]), b"")
        for n in names:
            files[n] = (files[n][0], content)
    return files


def _install_rpm(data: bytes, destination: Path, keep) -> None:
    for name, (mode, body) in rpm_files(data).items():
        if not keep(name):
            continue
        path = destination / name
        path.parent.mkdir(parents=True, exist_ok=True)
        if mode & 0o170000 == 0o120000:
            path.symlink_to(body.decode())
        else:
            path.write_bytes(body)
            path.chmod(mode & 0o755 | 0o600)


class Builder:
    def __init__(self, link: Path, jobs: int):
        self.link, self.jobs = link, jobs
        self.lib, self.xlib, self.bin = link / "lib", link / "xlib", link / "bin"
        self.src, self.log = link / "src", link / "build.log"
        self.stage1 = link / "gcc-build/gcc"
        self.run_path = f"{tc.SYSROOT}/lib:{tc.SYSROOT}/usr/lib:{self.xlib}"

    # -- compilers -----------------------------------------------------------------------------
    def _driver_script(self, driver: Path, prefixes: list[Path], includes: list[Path], extra: str = "") -> str:
        """A compiler driver that can also link and run configure's test programs through the
        staged loader and libraries; the link flags never reach the objects that are kept."""
        b = " ".join(f"-B{p}/" for p in prefixes)
        isystem = " ".join(f"-isystem {p}" for p in includes)
        return ("#!/bin/sh\n"
                f'LINKFLAGS="-Wl,-dynamic-linker,{tc.LOADER} -Wl,-rpath,{self.run_path} '
                f'-Wl,-rpath-link,{self.run_path}"\n'
                'for a in "$@"; do case "$a" in -c|-S|-E|-M|-MM) LINKFLAGS=;; esac; done\n'
                f'exec env -i PATH={self.link}/libexec:/run/current-system/sw/bin:/usr/bin:/bin '
                f'TMPDIR="${{TMPDIR:-/tmp}}" "{tc.LOADER}" --library-path "{tc.SYSROOT}/lib:{tc.SYSROOT}/usr/lib" '
                f'"{driver}" {b} -nostdinc {isystem} -L{self.lib} -L{self.xlib} -L{tc.SYSROOT}/usr/lib '
                f'-L{tc.SYSROOT}/lib {extra} $LINKFLAGS "$@"\n')

    def write_compilers(self) -> None:
        gcc_lib = tc._gcc_lib()
        prefixes = [gcc_lib, tc.SYSROOT / "usr/lib", self.link / "libexec", tc.WRAPPERS]
        includes = [gcc_lib / "include", tc.SYSROOT / "usr/include"]
        for name, driver in (("cc", "gcc"), ("c++", "g++")):
            program = tc.SYSROOT / "usr/bin" / driver
            tc._script(self.bin / name, self._driver_script(program, prefixes, includes))
            tc._script(self.bin / f"{name}-i686", self._driver_script(program, prefixes, includes,
                                                                      GCC_CFLAGS_I686))
            # The stage-1 cc1/cc1plus that gcc_runtime builds: the driver searches the last -B first.
            tc._script(self.bin / f"{name}-i686-stage1", self._driver_script(
                program, [*prefixes, self.stage1], includes, GCC_CFLAGS_I686))
        # libxml and libglade were compiled against Red Hat 6.0's glibc 2.1.1-6 headers: its
        # <bits/string2.h> turns every memset(p, 0, n) into a __bzero call (no __builtin_memset
        # macro, no small-size inline memset), as the image's libxml members call __bzero and
        # memset. The potato headers stay underneath for the kernel headers.
        tc._script(self.bin / "cc-i686-rh60", self._driver_script(
            tc.SYSROOT / "usr/bin/gcc", prefixes, [gcc_lib / "include", self.link / "rh60-include"],
            GCC_CFLAGS_I686))
        egcs = self.link / "egcs"
        wrappers = egcs / "libexec"
        for name in ("cpp", "cc1", "collect2"):
            tc._script(wrappers / name, tc._wrapper(egcs / EGCS_LIB / name))
        # Red Hat 6.2's assembler (binutils 2.9.5.0.22) beside its egcs: the image's GTK+/GDK use the
        # short `a0`/`a2` moffs forms for absolute byte moves and its nop fills, which the
        # project objects' as 2.9.1.0.25 never emits.
        redhat = self.link / "redhat-binutils"
        tc._script(wrappers / "as", tc._wrapper(redhat / "usr/bin/as", redhat / "usr/lib"))
        shutil.copyfile(self.link / "libexec/ld", wrappers / "ld")
        (wrappers / "ld").chmod(0o755)
        tc._script(self.bin / "egcc", self._driver_script(
            egcs / "usr/bin/gcc", [egcs / EGCS_LIB, tc.SYSROOT / "usr/lib", wrappers],
            [egcs / EGCS_LIB / "include", tc.SYSROOT / "usr/include"]))

    # -- running -------------------------------------------------------------------------------
    def env(self, cc: str, cflags: str, **extra: str) -> dict[str, str]:
        env = {**os.environ, "PATH": f"{self.bin}:{os.environ.get('PATH', '/usr/bin:/bin')}",
               "CC": str(self.bin / cc), "CFLAGS": cflags, "LC_ALL": "C", **extra}
        # The Nix shell exports its own toolchain (AR, LD, NM, ...); none of it may reach the build.
        for name in ("CONFIG_SITE", "CXX", "CXXFLAGS", "CPPFLAGS", "LDFLAGS", "MAKEFLAGS", "MFLAGS", "AR", "AS",
                     "LD", "NM", "RANLIB", "STRIP", "OBJCOPY", "OBJDUMP", "READELF", "CPP", "SIZE", "STRINGS",
                     "NIX_CFLAGS_COMPILE", "NIX_LDFLAGS"):
            if name not in extra:
                env.pop(name, None)
        return env

    def run(self, command: list[str], cwd: Path, env: dict[str, str]) -> None:
        with self.log.open("a") as stream:
            stream.write(f"$ (cd {cwd}) " + " ".join(command) + "\n")
            stream.flush()
            completed = subprocess.run(command, cwd=cwd, env=env, stdout=stream, stderr=subprocess.STDOUT)
        if completed.returncode:
            raise BuildError(f"{' '.join(command[:3])} failed in {cwd} (see {self.log})")

    def unpack(self, archive: bytes) -> Path:
        return tc._unpack_source(archive, self.src)

    # -- libraries -----------------------------------------------------------------------------
    def zlib(self, source: bytes) -> None:
        tree = self.unpack(source)
        env = self.env("egcc", ZLIB_CFLAGS)
        self.run(["./configure"], tree, env)
        self.run(["make", f"-j{self.jobs}", f"CC={self.bin / 'egcc'}", f"CFLAGS={ZLIB_CFLAGS}", "libz.a"],
                 tree, env)
        shutil.copyfile(tree / "libz.a", self.lib / "libz.a")
        for header in ("zlib.h", "zconf.h"):
            shutil.copyfile(tree / header, self.link / "include" / header)

    def glib(self, source: bytes) -> Path:
        tree = self.unpack(source)
        env = self.env("egcc", REDHAT_CFLAGS)
        prefix = self.link / "glib"
        self.run(["./configure", f"--prefix={prefix}", "--disable-shared", "i386-redhat-linux"], tree, env)
        self.run(["make", f"-j{self.jobs}"], tree, env)
        self.run(["make", "install"], tree, env)
        for name in ("libglib.a", "libgmodule.a"):
            shutil.copyfile(prefix / "lib" / name, self.lib / name)
        return prefix

    def gtk(self, source: bytes, glib_prefix: Path, x_headers: Path, patches: list[bytes]) -> None:
        tree = self.unpack(source)
        for patch in patches:
            completed = subprocess.run(["patch", "-p1", "--no-backup-if-mismatch"], cwd=tree, input=patch,
                                       capture_output=True)
            with self.log.open("ab") as stream:
                stream.write(b"$ patch -p1\n" + completed.stdout + completed.stderr)
            if completed.returncode:
                raise BuildError(f"GTK+ patch failed in {tree} (see {self.log})")
        env = self.env("egcc", REDHAT_CFLAGS)
        # Red Hat's configure macro: --sysconfdir=/etc (the image's gtkrc reads "/etc" + "/gtk/gtkrc").
        self.run(["./configure", "--prefix=/usr", "--sysconfdir=/etc", "--disable-shared",
                  f"--with-glib-prefix={glib_prefix}",
                  "--with-xinput=xfree", f"--x-includes={x_headers}", f"--x-libraries={self.xlib}",
                  "i386-redhat-linux"], tree, env)
        self.run(["make", f"-j{self.jobs}", "libgdk.la"], tree / "gdk", env)
        self.run(["make", f"-j{self.jobs}", "libgtk.la"], tree / "gtk", env)
        shutil.copyfile(tree / "gdk/.libs/libgdk.a", self.lib / "libgdk.a")
        shutil.copyfile(tree / "gtk/.libs/libgtk.a", self.lib / "libgtk.a")

    def gcc_runtime(self, source: bytes) -> None:
        """libgcc.a, crtbegin.o, crtend.o and libstdc++.a as a plain `make` of an i686-pc-linux-gnu
        2.95.2 tree builds them: its stage-1 compilers, built by the host's egcs 1.1.2 at
        configure's default CFLAGS for a GCC host (`-g -O2`)."""
        top = self.unpack(source)
        objdir = self.link / "gcc-build"
        objdir.mkdir()
        host = "i686-pc-linux-gnu"
        # A native configure finds the host's /usr/bin/as and enables its .balign/.p2align (with
        # maximum skip) features; without them labels lose their `.p2align 4,,7`.
        env = self.env("egcc", "-g -O2", AS=str(self.link / "egcs/libexec/as"))
        self.run([str(top / "configure"), "--prefix=/usr", "--enable-languages=c,c++", host], objdir, env)
        gcc_dir = objdir / "gcc"
        assert gcc_dir == self.stage1
        # The top-level make passes its CFLAGS (configure's -g -O2) down; gcc/Makefile alone says -g.
        self.run(["make", f"-j{self.jobs}", "all-libiberty"], objdir, env)
        self.run(["make", f"-j{self.jobs}", "CFLAGS=-g -O2", "cc1", "cc1plus"], gcc_dir, env)
        self.libgcc(top / "gcc", gcc_dir)
        target = objdir / host
        cc, cxx = self.bin / "cc-i686-stage1", self.bin / "c++-i686-stage1"
        target_env = self.env(cc.name, "-g -O2", CXX=str(cxx),
                              CXXFLAGS="-g -O2 -fvtable-thunks -D_GNU_SOURCE", AR="ar", RANLIB="ranlib")
        make_vars = [f"CC={cc}", f"CXX={cxx}", "CFLAGS=-g -O2",
                     "CXXFLAGS=-g -O2 -fvtable-thunks -D_GNU_SOURCE"]
        for directory in ("libiberty", "libio", "libstdc++"):
            build = target / directory
            build.mkdir(parents=True)
            self.run(["sh", str(top / "configure"), f"--host={host}", f"--build={host}", "--enable-multilib",
                      "--prefix=/usr", "--enable-languages=c,c++", f"--srcdir={top / directory}",
                      f"--with-target-subdir={host}"], build, {**target_env, "CONFIG_SITE": "no-such-file"})
            self.run(["make", f"-j{self.jobs}", *make_vars], build, target_env)
        shutil.copyfile(target / "libstdc++/libstdc++.a.2.10.0", self.lib / "libstdc++.a")

    def libgcc(self, source: Path, gcc_dir: Path) -> None:
        """The libgcc2.a and crtstuff rules of gcc/Makefile, with the stage-1 compilers for xgcc."""
        makefile = (gcc_dir / "Makefile").read_text()
        joined = makefile.replace("\\\n", " ")

        def variable(name: str) -> list[str]:
            # The last assignment wins (config/t-linux overrides the defaults).
            matches = re.findall(rf"^{name}\s*=(.*)$", joined, re.M)
            return matches[-1].split() if matches else []

        cc = str(self.bin / "cc-i686-stage1")
        gcc_cflags = ["-DIN_GCC", "-g", "-O2", "-I./include"]
        libgcc2 = ["-O2", *gcc_cflags, *variable("TARGET_LIBGCC2_CFLAGS"), *variable("LIBGCC2_DEBUG_CFLAGS"),
                   "-DIN_LIBGCC2", "-D__GCC_FLOAT_NOT_NEEDED"]
        includes = ["-I.", f"-I{source}", f"-I{source}/config", f"-I{source.parent}/include"]
        out = gcc_dir / "libgcc-objs"
        out.mkdir()
        env = self.env("cc-i686-stage1", "-g -O2")
        members = []

        def compile_(name: str, *arguments: str, source_file: Path = source / "libgcc2.c") -> None:
            self.run([cc, *libgcc2, *includes, *arguments, "-c", str(source_file), "-o", str(out / f"{name}.o")],
                     gcc_dir, env)
            members.append(f"{name}.o")

        for name in variable("LIB2FUNCS"):
            compile_(name, f"-DL{name}")
        for name in variable("LIB2FUNCS_EH"):
            compile_(name, "-fexceptions", f"-DL{name}")
        compile_("frame", source_file=source / "frame.c")
        cp = ["-I" + str(source / "cp/inc")]
        for name, file, defines in (("tinfo", "tinfo.cc", ()), ("tinfo2", "tinfo2.cc", ()), ("new", "new.cc", ()),
                                    ("opnew", "new1.cc", ("-DL_op_new",)), ("opnewnt", "new1.cc", ("-DL_op_newnt",)),
                                    ("opvnew", "new2.cc", ("-DL_op_vnew",)), ("opvnewnt", "new2.cc", ("-DL_op_vnewnt",)),
                                    ("opdel", "new2.cc", ("-DL_op_delete",)), ("opdelnt", "new2.cc", ("-DL_op_delnt",)),
                                    ("opvdel", "new2.cc", ("-DL_op_vdel",)), ("opvdelnt", "new2.cc", ("-DL_op_vdelnt",)),
                                    ("exception", "exception.cc", ("-fexceptions",))):
            compile_(name, "-g", "-O2", *cp, *defines, source_file=source / "cp" / file)
        # gcc/Makefile's libgcc.a rule re-archives the extracted members as `*.o`, in name order:
        # the image links _eh, _pure, _udivdi3, _umoddi3, exception, frame, new, opdel, ... tinfo2.
        archive = self.lib / "libgcc.a"
        self.run(["ar", "rc", str(archive), *sorted(members)], out, env)
        self.run(["ranlib", str(archive)], out, env)
        crt = [cc, *gcc_cflags, *includes, "-g0", "-finhibit-size-directive", "-fno-inline-functions",
               "-fno-exceptions", *variable("CRTSTUFF_T_CFLAGS")]
        for part in ("BEGIN", "END"):
            self.run([*crt, "-c", str(source / "crtstuff.c"), f"-DCRT_{part}", "-o",
                      str(self.lib / f"crt{part.lower()}.o")], gcc_dir, env)

    def libxml_and_libglade(self, xml_source: bytes, glade_source: bytes) -> None:
        # libglade's configure asks gtk-config/glib-config: the compile toolchain's GTK+ 1.2.8
        # headers (as for the editor's own units) and the libraries just built.
        cflags = f"-I{tc.GTK}/usr/include -I{tc.GTK}/usr/lib/glib/include"
        for name, libraries in (("glib-config", f"-L{self.lib} -rdynamic -lgmodule -lglib -ldl"),
                                ("gtk-config", f"-L{self.lib} -L{self.xlib} -lgtk -lgdk -rdynamic -lgmodule "
                                               "-lglib -ldl -lXi -lXext -lX11 -lm")):
            tc._script(self.bin / name, "#!/bin/sh\nfor a in \"$@\"; do case $a in --version) echo 1.2.8;; "
                       f"--cflags) echo \"{cflags}\";; --libs) echo \"{libraries}\";; esac; done\n")
        env = self.env("cc-i686-rh60", XML_GLADE_CFLAGS, CPPFLAGS=f"-I{self.link}/include",
                       LDFLAGS=f"-L{self.lib}")
        configure = ["./configure", f"--prefix={self.link}", "--disable-shared", "--enable-static",
                     "i686-pc-linux-gnu"]
        xml = self.unpack(xml_source)
        self.run(configure, xml, env)
        self.run(["make", f"-j{self.jobs}", "libxml.la", "xml-config"], xml, env)
        self.run(["make", "install-libLTLIBRARIES", "install-xmlincHEADERS", "install-binSCRIPTS"], xml, env)
        glade = self.unpack(glade_source)
        self.run([*configure, "--without-gnome", "--disable-bonobo"], glade, env)
        self.run(["make", f"-j{self.jobs}", "libglade.la"], glade / "glade", env)
        shutil.copyfile(glade / "glade/.libs/libglade.a", self.lib / "libglade.a")


def build(link: Path, media: dict[str, bytes], jobs: int = 2) -> None:
    builder = Builder(link, jobs)
    lib, xlib = builder.lib, builder.xlib
    bfd = "usr/lib/libbfd-2.9.1.0.25.so"
    tc._unpack(media["binutils.tgz"], {"usr/bin/ld": link / "binutils/usr/bin/ld", bfd: link / "binutils" / bfd})
    tc._script(link / "libexec/ld", tc._wrapper(link / "binutils/usr/bin/ld", link / "binutils/usr/lib"))
    tc._unpack(media["glibc.tgz"], {f"usr/lib/{name}": lib / name
                                    for name in ("crt1.o", "crti.o", "crtn.o", "libc_nonshared.a")})
    # libc.so is a linker script naming /lib/libc.so.6; ld 2.9.1 has no --sysroot.
    (lib / "libc.so").write_text(f"/* GNU ld script (staged) */\n"
                                 f"GROUP ( {tc.SYSROOT}/lib/libc.so.6 {lib}/libc_nonshared.a )\n")
    # The image's link saw Red Hat 6.0's shared glibc 2.1.1-6: every imported libc, libm and
    # libdl symbol carries its st_size (dlerror 192, dlclose 55, dlsym 79, dlopen 61; potato's
    # 2.1.3 differs). link/glibc comes first on h3maped's -L path only; the library builds keep
    # potato's.
    glibc = link / "glibc"
    names = {"lib/libc-2.1.1.so": "libc.so.6", "lib/libm-2.1.1.so": "libm.so.6", "lib/libdl-2.1.1.so": "libdl.so.2"}
    _install_rpm(media["glibc-2.1.1-6.i386.rpm"], link / "glibc-rpm", lambda name: name in names)
    glibc.mkdir(parents=True, exist_ok=True)
    for member, soname in names.items():
        shutil.copyfile(link / "glibc-rpm" / member, glibc / soname)
    for stem, soname in (("libm", "libm.so.6"), ("libdl", "libdl.so.2")):
        (glibc / f"{stem}.so").symlink_to(soname)
    (glibc / "libc.so").write_text(f"/* GNU ld script (staged) */\n"
                                   f"GROUP ( {glibc}/libc.so.6 {lib}/libc_nonshared.a )\n")
    # libX11 and libXi of Red Hat 6.2 (XFree86 3.3.6), libXext of Red Hat 7.0 (XFree86 4.0.1, built
    # against glibc 2.0 symbols only): ld records the X imports in these libraries' .dynsym order,
    # and the image's .dynsym places XGetVisualInfo between libXext's XShmDetach and XIfEvent, an
    # undefined reference only XFree86 4's libXext (its XEVI client) has.
    xlib.mkdir(parents=True, exist_ok=True)
    for package, names in (("XFree86-libs-3.3.6-20.i386.rpm", ("libX11.so.6.1", "libXi.so.6.0")),
                           ("XFree86-libs-4.0.1-1.i386.rpm", ("libXext.so.6.4",))):
        _install_rpm(media[package], xlib.parent / "xlibs-rpm",
                     lambda name, names=names: name in {f"usr/X11R6/lib/{x}" for x in names})
        for name in names:
            shutil.copyfile(xlib.parent / "xlibs-rpm/usr/X11R6/lib" / name, xlib / name)
    for name, stem in X_LIBRARIES.items():
        for alias in (f"{stem}.so.6", f"{stem}.so"):
            (xlib / alias).symlink_to(name)
    x_root = link / "x11"
    _install_rpm(media["XFree86-devel-3.3.6-20.i386.rpm"], x_root,
                 lambda name: name.startswith("usr/X11R6/include/"))
    _install_rpm(media["binutils-2.9.5.0.22-6.i386.rpm"], link / "redhat-binutils",
                 lambda name: name in ("usr/bin/as", "usr/lib/libbfd-2.9.5.0.22.so"))
    shutil.copytree(tc.SYSROOT / "usr/include", link / "rh60-include", symlinks=True)
    _install_rpm(media["glibc-devel-2.1.1-6.i386.rpm"], link / "rh60-glibc",
                 lambda name: name.startswith("usr/include/"))
    shutil.copytree(link / "rh60-glibc/usr/include", link / "rh60-include", symlinks=True, dirs_exist_ok=True)
    egcs = link / "egcs"
    for package in ("egcs-1.1.2-30.i386.rpm", "cpp-1.1.2-30.i386.rpm"):
        _install_rpm(media[package], egcs,
                     lambda name: name.startswith(EGCS_LIB + "/") or name == "usr/bin/gcc")
    # gettext's tools on PATH while GTK+ configures: NLS on (bindtextdomain, _nl_msg_cat_cntr).
    tools = ("msgfmt", "xgettext", "gettext")
    tc._unpack(media["gettext.tgz"], {f"usr/bin/{name}": link / "gettext/usr/bin" / name for name in tools})
    for name in tools:
        tc._script(builder.bin / name, tc._wrapper(link / "gettext/usr/bin" / name))
    (link / "include").mkdir(parents=True, exist_ok=True)
    builder.write_compilers()
    builder.zlib(media["zlib-1.0.8.tar.gz"])
    glib_prefix = builder.glib(media["glib-1.2.8.tar.gz"])
    # Red Hat's i18n patch (Akira Higuchi, 1999; Fedora keeps its 1.2.10 refresh): the word
    # breaks of GtkEntry, GtkLabel and GtkText call iswpunct/iswcntrl, as the image imports them.
    builder.gtk(media["gtk+-1.2.8.tar.gz"], glib_prefix, x_root / "usr/X11R6/include",
                [media["gtk+-1.2.10-ahiguti.patch"]])
    builder.gcc_runtime(media["gcc-2.95.2.tar.gz"])
    builder.libxml_and_libglade(media["libxml-1.8.9.tar.gz"], media["libglade-0.14.tar.gz"])
