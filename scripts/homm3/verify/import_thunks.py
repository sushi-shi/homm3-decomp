"""Verify linker import thunks in retail `.text`.

A function imported without `__declspec(dllimport)` is called through a
six-byte linker thunk, `jmp dword ptr [__imp_X]`. A reviewed census row is
credited as `import-thunk` only when its bytes are exactly `FF 25` followed by
the address of an IAT slot named by the PE import directory, and the reviewed
relocation inventory has the operand site. For a system DLL the pinned VC6
import library must also provide that import: a short-import member (for
which LINK synthesizes the thunk) or a long-format member whose `.text` is the
thunk template. Vendor DLLs have no pinned library; their thunks rest on the
import directory alone and say so in the report.
"""
from __future__ import annotations

import struct
from pathlib import Path

from homm3.delink.implib import _ar_members, _short_import

THUNK = b'\xff\x25'
SIZE = 6


def _undecorate(symbol, name_type):
    """The exported name a short import binds (IMPORT_OBJECT_NAME_TYPE)."""
    if name_type == 1:
        return symbol
    name = symbol.lstrip('?@_') if name_type in (2, 3) else symbol
    if name_type == 3:
        name = name.split('@', 1)[0]
    return name


def _long_member(body):
    """(thunk-ok, imported name or ordinal) for a long-format member."""
    if len(body) < 20 or struct.unpack_from('<H', body, 0)[0] != 0x014c:
        return False, None
    count, optional = struct.unpack_from('<H', body, 2)[0], struct.unpack_from('<H', body, 16)[0]
    thunk, imported = False, None
    for index in range(count):
        offset = 20 + optional + index * 40
        name = body[offset:offset + 8].rstrip(b'\0').decode('latin1', 'replace')
        size, pointer = struct.unpack_from('<II', body, offset + 16)
        payload = body[pointer:pointer + size] if pointer else b''
        if name == '.text' and payload == THUNK + bytes(4):
            thunk = True
        elif name == '.idata$6' and len(payload) > 2:
            imported = payload[2:].split(b'\0', 1)[0].decode('latin1')
        elif name == '.idata$4' and len(payload) >= 4:
            value = struct.unpack_from('<I', payload, 0)[0]
            if value & 0x80000000:
                imported = value & 0xffff
    return thunk, imported


def library_imports(directory: Path, dll: str):
    """{name or ordinal} importable through a thunk from the pinned library."""
    stem = Path(dll).stem.lower()
    paths = [p for p in directory.iterdir() if p.suffix.lower() == '.lib'
             and p.stem.lower() == stem] if directory.is_dir() else []
    found = set()
    for path in paths:
        for _, body in _ar_members(path):
            short = _short_import(body)
            if short is not None:
                symbol, owner, value, name_type = short
                if owner.lower() == dll.lower():
                    found.add(value if name_type == 0 else _undecorate(symbol, name_type))
                continue
            thunk, imported = _long_member(body)
            if thunk and imported is not None:
                found.add(imported)
    return found, bool(paths)


def compare(project, pe, sizes=None, claimed=()):
    """Return verified import thunks and census rows that do not verify.

    Rows already owned elsewhere (a source body compiled to the same jump, or
    a reviewed runtime placement) are left to that owner.
    """
    from homm3.delink.image import Image
    from homm3.retail_labels.censuses import functions
    image = Image(pe)
    sizes = sizes or {row['rva']: row['size'] for row in functions()}
    slots = {slot: (name, dll, ordinal) for slot, name, dll, ordinal in image.import_slots()}
    directory = project.toolchain / 'lib'
    libraries = {}
    result = dict(matches=[], gaps=[])
    for rva, size in sorted(sizes.items()):
        if size != SIZE or rva in claimed:
            continue
        body = pe.read(rva, SIZE)
        if body is None or body[:2] != THUNK:
            continue
        slot = struct.unpack_from('<I', body, 2)[0] - image.image_base
        row = dict(rva=rva, size=SIZE, slot=slot)
        if slot not in slots:
            result['gaps'].append(dict(row, reason='operand is not an import slot')); continue
        name, dll, ordinal = slots[slot]
        imported = ordinal if name is None else name
        row.update(dll=dll, imported=imported)
        if image.relocs_in(rva, rva + SIZE) != [rva + 2]:
            result['gaps'].append(dict(row, reason='operand relocation site not admitted'))
            continue
        if dll.lower() not in libraries:
            libraries[dll.lower()] = library_imports(directory, dll)
        provided, pinned = libraries[dll.lower()]
        if pinned and imported not in provided:
            result['gaps'].append(dict(row, reason='pinned import library lacks the import'))
            continue
        row['evidence'] = 'pinned-import-library' if pinned else 'retail-import-directory'
        result['matches'].append(row)
    return result
