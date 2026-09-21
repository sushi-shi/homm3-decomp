"""Rebuild third-party import libraries from the pinned executable's imports.

No SDK binaries or retail code are copied into the candidate. The generated
libraries contain ordinary loader imports for the same DLL/export identities.
"""
from __future__ import annotations

from pathlib import Path
import struct
import subprocess

from homm3.core.pe_layout import Layout

VENDOR_DLLS = ('BINKW32.DLL', 'MSS32.DLL', 'SMACKW32.DLL', 'IFC20.dll')


def named_imports(data: bytes) -> dict[str, list[str | int]]:
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
    result: dict[str, list[str | int]] = {}
    for displacement in range(0, size - 19, 20):
        original, timestamp, chain, name, iat = struct.unpack_from(
            '<5I', data, offset(directory + displacement, 20))
        if not any((original, timestamp, chain, name, iat)):
            return result
        dll = string(name)
        names = result.setdefault(dll, [])
        table = original or iat
        while True:
            thunk, = struct.unpack_from('<I', data, offset(table, 4))
            table += 4
            if not thunk:
                break
            names.append(thunk & 0xffff if thunk & 0x80000000 else string(thunk + 2))
    raise ValueError('unterminated import directory')


def exact_import_names(archive: bytes, dll: str, names: list[str]) -> bytes:
    """Keep the compiler and loader names identical in COFF short imports.

    dlltool's i386 mode defaults to IMPORT_NAME_NOPREFIX even with
    --no-leading-underscore. Our source names are already the complete
    decorated DLL exports, so select IMPORT_NAME instead. Member lengths,
    linker indexes and descriptor objects are unchanged.
    """
    if not archive.startswith(b"!<arch>\n"):
        raise ValueError("expected COFF archive")
    result = bytearray(archive)
    found = []
    position = 8
    while position < len(archive):
        header = archive[position:position + 60]
        if len(header) != 60 or header[58:] != b"`\n":
            raise ValueError("invalid archive member")
        size = int(header[48:58])
        start = position + 60
        member = archive[start:start + size]
        if len(member) != size:
            raise ValueError("truncated archive member")
        if member.startswith(b"\0\0\xff\xff"):
            if len(member) < 20:
                raise ValueError("truncated short import")
            symbol, owner, tail = member[20:].split(b"\0", 2)
            if owner.decode('ascii').lower() != dll.lower() or tail:
                raise ValueError("unexpected short import identity")
            found.append(symbol.decode('ascii'))
            flags, = struct.unpack_from('<H', member, 18)
            struct.pack_into('<H', result, start + 18, (flags & 3) | (1 << 2))
        position = start + size + (size & 1)
    if sorted(found) != sorted(names):
        raise ValueError(f"{dll}: generated import symbols differ from retail")
    return bytes(result)


def build_vendor_libraries(data: bytes, directory: Path) -> list[Path]:
    imports = {dll.lower(): names for dll, names in named_imports(data).items()}
    directory.mkdir(parents=True, exist_ok=True)
    libraries = []
    for dll in VENDOR_DLLS:
        names = imports[dll.lower()]
        if any(isinstance(name, int) for name in names):
            raise ValueError(f'{dll}: ordinal imports need an explicit ABI mapping')
        exports = [f'  {name}' + (' DATA' if name == '?m_dwErrHandlingFlags@CIFCErrors@@0KA' else '')
                   for name in names]
        definition = directory / (Path(dll).stem + '.def')
        definition.write_text(f'LIBRARY {dll}\nEXPORTS\n' + '\n'.join(exports) + '\n')
        library = definition.with_suffix('.lib')
        library.unlink(missing_ok=True)
        subprocess.run(['llvm-dlltool', '-m', 'i386', '--no-leading-underscore',
                        '-d', str(definition), '-l', str(library)], check=True)
        library.write_bytes(exact_import_names(library.read_bytes(), dll, names))
        libraries.append(library)
    return libraries
