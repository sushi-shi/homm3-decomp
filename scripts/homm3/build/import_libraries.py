"""homm3.build.import_libraries - the vendor import libraries the toolchain lacks.

    python3 -m homm3.build.import_libraries [--out-dir build/exe/imports]

Retail imports mss32.dll, binkw32.dll, smackw32.dll and IFC20.dll, whose SDK
import libraries are not pinned. They are rebuilt from the retail import
table, which is ground truth for what the original libraries produced: each
hint/name string (`_AIL_startup@0`, `?Close@CImmProject@@QAEXXZ`) and each
hint. No SDK binary is copied; nothing here needs the vendor DLLs.

How (HoMM1's method, homm1.graph.implib): the vendor built each library
alongside its DLL, so the library is the `/IMPLIB` of a stub DLL whose
exports decorate to exactly the retail names. `LIB /DEF` cannot do it: for
`_AIL_startup@0` it either prefixes another underscore to the public symbol
or records an undecorating (NOPREFIX) import name. A `__declspec(dllexport)
__stdcall` function with the matching argument bytes exports
`_AIL_startup@0` under that public symbol with an as-is import name.
IFC20's exports are C++ members, which `LIB /DEF` records exactly (public
symbol and import name are the mangled name), so IFC20's library is the
`LIB /DEF` of its sorted export table.

Hints: a hint is the export's index in the DLL's sorted export-name table,
so the vendor's library carried the index each name had in the real DLL's
full export list, and retail's import table stores those values. The stub
reproduces them with filler exports that sort strictly between the real
names, one per unclaimed index; nothing references a filler, so none
reaches the image. Every produced library is re-read and its import names,
hints and DLL name are checked against retail.

The linker that wrote each library is a reviewed fact,
config/retail/import-libraries.tsv.
"""
from __future__ import annotations

import argparse
import re
import struct
import sys
from pathlib import Path

from homm3.core import common
from homm3.core.pe_layout import Layout

ROOT = common.HOMM3_DIR
FORMATS = ROOT / "config/retail/import-libraries.tsv"
VENDOR_DLLS = ('BINKW32.DLL', 'MSS32.DLL', 'SMACKW32.DLL', 'IFC20.dll')

#: `_name@n` = __stdcall with n argument bytes.
STDCALL = re.compile(r"^_(?P<name>[A-Za-z_][A-Za-z0-9_]*)@(?P<bytes>\d+)$")
#: Filler identifiers: valid in C and in a .def, sorting between real names.
_IDENT = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz"


def import_table(data: bytes) -> dict[str, list[tuple[str | int, int]]]:
    """{dll: [(hint/name string or ordinal, hint)]} in import-table order."""
    layout = Layout.parse(data)

    def offset(rva: int, size: int = 1) -> int:
        for section in layout.sections:
            if section.rva <= rva and rva + size <= section.rva + section.raw_size:
                return section.raw_offset + rva - section.rva
        raise ValueError(f'import RVA {rva:#x}+{size:#x} is not file backed')

    def string(rva: int) -> str:
        start = offset(rva)
        section = next(s for s in layout.sections if s.rva <= rva < s.rva + s.raw_size)
        end = data.find(b'\0', start, section.raw_offset + section.raw_size)
        if end < 0:
            raise ValueError('unterminated import name')
        return data[start:end].decode('ascii')

    pe, = struct.unpack_from('<I', data, 0x3c)
    directory, size = struct.unpack_from('<II', data, pe + 24 + 104)
    if not directory:
        return {}
    result: dict[str, list[tuple[str | int, int]]] = {}
    for displacement in range(0, size - 19, 20):
        original, timestamp, chain, name, iat = struct.unpack_from(
            '<5I', data, offset(directory + displacement, 20))
        if not any((original, timestamp, chain, name, iat)):
            return result
        names = result.setdefault(string(name), [])
        table = original or iat
        while True:
            thunk, = struct.unpack_from('<I', data, offset(table, 4))
            table += 4
            if not thunk:
                break
            if thunk & 0x80000000:
                names.append((thunk & 0xffff, -1))
            else:
                hint, = struct.unpack_from('<H', data, offset(thunk, 2))
                names.append((string(thunk + 2), hint))
    raise ValueError('unterminated import directory')


def named_imports(data: bytes) -> dict[str, list[str | int]]:
    return {dll: [name for name, _hint in rows] for dll, rows in import_table(data).items()}


def formats() -> dict[str, str]:
    """{dll lower: linker} from the reviewed table."""
    from homm3.core.tsv import read as read_tsv
    return {row["dll"].lower(): row["linker"] for row in read_tsv(FORMATS)[2]}


# --------------------------------------------------------------------- stub --

def _gap_base(a: str | None, b: str) -> str:
    """An identifier `base` with a < base + <digits> < b (bytewise)."""
    ident = sorted(_IDENT)
    if a is None:
        for j in range(len(b)):
            lower = [c for c in ident if c < b[j]]
            if lower:
                return b[:j] + lower[-1]
        raise ValueError(f"no filler name sorts below {b!r}")
    i = 0
    while i < len(a) and i < len(b) and a[i] == b[i]:
        i += 1
    if i >= len(b):
        raise ValueError(f"{a!r} does not sort below {b!r}")
    middle = [c for c in ident if (i >= len(a) or c > a[i]) and c < b[i]]
    if middle:
        base = a[:i] + middle[0]
    else:
        after = [c for c in ident if c > (a[i + 1] if i + 1 < len(a) else "")]
        base = a[:i + 1] + after[0]
    if not a < base < b:
        raise ValueError(f"cannot split {a!r} .. {b!r}")
    return base


def export_table(hints: dict[str, int], prefix: str = "") -> list[tuple[str, bool]]:
    """[(export name, filler)] in sorted order, padded so each real name sits
    at its retail hint. With a `prefix` every name carries (C++ names' `?`),
    fillers are built after it, so LIB keeps them undecorated and they sort
    among the real names."""
    real = sorted(hints)
    values = [hints[name] for name in real]
    if values != sorted(values) or len(set(values)) != len(values):
        raise ValueError("retail hints are not ascending in sorted-name order")
    table: list[tuple[str, bool]] = []
    previous, position = None, 0
    for name, hint in zip(real, values):
        missing = hint - position
        if missing:
            cut = len(prefix)
            base = prefix + _gap_base(previous and previous[cut:], name[cut:])
            width = len(str(missing - 1))
            table += [(f"{base}{i:0{width}d}", True) for i in range(missing)]
            position += missing
        table.append((name, False))
        previous = name
        position += 1
    flat = [name for name, _filler in table]
    if any(x >= y for x, y in zip(flat, flat[1:])):
        raise ValueError("export table is not strictly sorted")
    return table


def stub_sources(dll: str, hints: dict[str, int], data_exports: set[str]) -> tuple[str, list[str]]:
    """(C stub source, .def EXPORTS entries) for a DLL exporting `hints`."""
    lines = [f"/* GENERATED by homm3.build.import_libraries: stub exports of {dll}. */"]
    entries = []
    for index, (name, filler) in enumerate(export_table(hints)):
        match = STDCALL.match(name)
        if filler:
            body = f"stub_filler_{index}"
            lines.append(f"void {body}(void) {{}}")
            entries.append(f"{name}={body}")
        elif match:
            count, rest = divmod(int(match["bytes"]), 4)
            if rest:
                raise ValueError(f"{dll}: {name} has a non-dword argument size")
            arguments = ", ".join(f"int a{i}" for i in range(count)) or "void"
            lines.append(f"__declspec(dllexport) void __stdcall "
                         f"{match['name']}({arguments}) {{}}")
        elif name.startswith("?"):
            body = f"stub_export_{index}"
            if name in data_exports:
                lines.append(f"int {body} = 0;")
                entries.append(f"{name}={body} DATA")
            else:
                lines.append(f"void {body}(void) {{}}")
                entries.append(f"{name}={body}")
        else:
            raise ValueError(f"{dll}: cannot export {name!r}")
    return "\n".join(lines) + "\n", entries


# ------------------------------------------------------------------- verify --

def verify_library(library: Path, dll: str, hints: dict[str, int]) -> None:
    """Fail unless every retail import is a member of `dll` whose import
    name is the retail string and whose hint is retail's."""
    from homm3.verify.library_code import archive_members
    got = {}
    for _name, body in archive_members(library):
        if body[:4] != b"\0\0\xff\xff":
            continue
        _version, _machine, _stamp, _size, hint, flags = struct.unpack_from("<HHHHIIHH", body)[2:]
        symbol, owner = body[20:].split(b"\0")[:2]
        kind = (flags >> 2) & 7
        name = symbol.decode("latin-1")
        if kind == 2:
            name = name[1:] if name[:1] in ("_", "@", "?") else name
        elif kind == 3:
            name = name.lstrip("_@?").split("@", 1)[0]
        if owner.decode("latin-1") != dll:
            raise ValueError(f"{library.name}: member names {owner!r}, not {dll}")
        if name in hints:
            got[name] = hint
    bad = {n: (hints[n], got.get(n)) for n in hints if got.get(n) != hints[n]}
    if bad:
        raise ValueError(f"{library.name}: import names or hints differ from retail: "
                         + ", ".join(f"{n} {w}/{g}" for n, (w, g) in list(bad.items())[:8]))


# -------------------------------------------------------------------- build --

def _linker(name: str) -> Path:
    from homm3.core.cc_wrap import compiler_dir, find_ci, msvc_dir
    root = msvc_dir() if name == "msvc6.0" else compiler_dir(name)
    path = find_ci(root / "bin", "link.exe")
    if path is None:
        raise ValueError(f"no LINK.EXE for {name}")
    return path


def synthesize(dll: str, hints: dict[str, int], data_exports: set[str],
               linker: str, directory: Path) -> Path:
    from homm3.build.link import run_wine
    from homm3.core.cc_wrap import find_ci, msvc_dir, winepath_w
    stem = Path(dll).stem.lower()
    work = directory / f"{stem}-stub"
    work.mkdir(parents=True, exist_ok=True)
    library = directory / f"{stem}.lib"
    staged = work / f"{stem}.lib"
    staged.unlink(missing_ok=True)
    definition = work / "stub.def"
    if all(name.startswith("?") for name in hints):
        # Decorated C++ names: LIB /DEF keeps them as they are.
        definition.write_text(f"LIBRARY {Path(dll).stem}\nEXPORTS\n" + "".join(
            f"    {name}{' DATA' if name in data_exports else ''}\n"
            for name, _filler in export_table(hints, prefix="?")))
        output, code = run_wine(["wine", str(_linker(linker)), "-lib", "/NOLOGO",
                                 "/MACHINE:IX86", f"/DEF:{winepath_w(definition)}",
                                 f"/NAME:{dll}", f"/OUT:{winepath_w(staged)}"], work)
    else:
        source, entries = stub_sources(dll, hints, data_exports)
        (work / "stub.c").write_text(source)
        definition.write_text(f"LIBRARY {Path(dll).stem}\nEXPORTS\n"
                              + "".join(f"    {e}\n" for e in entries))
        cl = find_ci(msvc_dir() / "bin", "cl.exe")
        (work / "stub.obj").unlink(missing_ok=True)
        output, code = run_wine(["wine", str(cl), "/nologo", "/c", "/Fostub.obj", "stub.c"], work)
        if code or not (work / "stub.obj").is_file():
            raise ValueError(f"{dll}: stub compile failed:\n{output[-2000:]}")
        # The stub's /OUT name is the DLL name LINK records in the descriptor
        # and as every member's name: retail's spelling.
        output, code = run_wine(["wine", str(_linker(linker)), "/NOLOGO", "/DLL", "/NOENTRY",
                                 "/NODEFAULTLIB", f"/DEF:{winepath_w(definition)}",
                                 f"/OUT:{winepath_w(work / dll)}",
                                 f"/IMPLIB:{winepath_w(staged)}", "stub.obj"], work)
    if code or not staged.is_file():
        raise ValueError(f"{dll}: import library build failed:\n{output[-2000:]}")
    verify_library(staged, dll, hints)
    staged.replace(library)
    return library


def build_vendor_libraries(data: bytes, directory: Path) -> list[Path]:
    """One import library per vendor DLL, named as retail spells the DLL
    (mss32.dll, binkw32.dll, smackw32.dll, IFC20.dll): LINK orders the IAT
    by that name, case-sensitively."""
    table = import_table(data)
    linkers = formats()
    directory.mkdir(parents=True, exist_ok=True)
    out = []
    for vendor in VENDOR_DLLS:
        dll = next(name for name in table if name.lower() == vendor.lower())
        rows = table[dll]
        if any(isinstance(name, int) for name, _hint in rows):
            raise ValueError(f"{dll}: ordinal imports need a reviewed symbol")
        hints = {name: hint for name, hint in rows}
        # C++ data members mangle with a storage digit after `@@`.
        data_exports = {name for name in hints if re.match(r"^\?\w+@\w+@@[0-2]", name)}
        out.append(synthesize(dll, hints, data_exports, linkers[dll.lower()], directory))
    return out


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out-dir", type=Path, default=ROOT / "build/exe/imports")
    args = parser.parse_args(argv)
    try:
        libraries = build_vendor_libraries(common.load_image()[0].data, args.out_dir)
    except (ValueError, OSError) as exc:
        print(f"[implib] {exc}", file=sys.stderr)
        return 1
    for library in libraries:
        print(f"[implib] {library} (import names and hints equal retail)")
    return 0


def logged_main(argv: list[str] | None = None) -> int:
    import shlex
    from homm3.core.usage import append, run_logged
    argv = list(sys.argv[1:] if argv is None else argv)
    command = shlex.join(["python3", "-m", "homm3.build.import_libraries", *argv])
    return run_logged(main, argv, lambda rc, **meta: append(
        ROOT / "build/homm3_usage.log", command, rc, **meta), failure_rc=1)


if __name__ == "__main__":
    sys.exit(logged_main())
