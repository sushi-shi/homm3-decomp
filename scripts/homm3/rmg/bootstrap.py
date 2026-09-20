"""Verified, disposable headless bootstrap for the pinned PE32 image."""
from __future__ import annotations

import struct

from homm3.core.pe_layout import Layout


def offset(data: bytes, va: int) -> int:
    layout = Layout.parse(data)
    rva = va - layout.image_base
    for section in layout.sections:
        if section.rva <= rva < section.rva + section.raw_size:
            return section.raw_offset + rva - section.rva
    raise ValueError(f"VA {va:#x} has no file-backed storage")


def imports(data: bytes) -> dict[str, int]:
    pe, = struct.unpack_from('<I', data, 0x3c)
    base = Layout.parse(data).image_base
    directory, = struct.unpack_from('<I', data, pe + 24 + 104)
    descriptor = offset(data, base + directory)
    result = {}
    while any(struct.unpack_from('<5I', data, descriptor)):
        original, _, _, _, iat = struct.unpack_from('<5I', data, descriptor)
        table = offset(data, base + (original or iat))
        index = 0
        while (entry := struct.unpack_from('<I', data, table + index * 4)[0]):
            if not entry & 0x80000000:
                start = offset(data, base + entry) + 2
                name = data[start:data.index(b'\0', start)].decode('ascii')
                result[name] = base + iat + index * 4
            index += 1
        descriptor += 20
    return result


def patch_winmain(data: bytes) -> bytes:
    """Keep the real entry/CRT; replace only WinMain with LoadLibrary/run.

    The caller verifies the complete executable hash before this operation.
    All strings and instructions fit inside WinMain's admitted 0x1cf span.
    """
    va, extent = 0x4f7a30, 0x1cf
    start = offset(data, va)
    iat = imports(data)
    code = bytearray()
    code += b'\x68' + struct.pack('<I', va + 0x80)
    code += b'\xff\x15' + struct.pack('<I', iat['LoadLibraryA'])
    code += b'\x85\xc0\x74\x17'
    code += b'\x68' + struct.pack('<I', va + 0xa0) + b'\x50'
    code += b'\xff\x15' + struct.pack('<I', iat['GetProcAddress'])
    code += b'\x85\xc0\x74\x07\xff\xd0\xc2\x10\x00'
    # Both failures return a distinct process error through the original CRT.
    failure = len(code)
    code += b'\xb8\x7e\x00\x00\x00\xc2\x10\x00'
    code[14] = failure - 15
    code[30] = failure - 31
    code.extend(b'\xcc' * (extent - len(code)))
    name = b'rmg-driver.dll\0'
    code[0x80:0x80 + len(name)] = name
    code[0xa0:0xa0 + 4] = b'run\0'
    if len(code) != extent:
        raise ValueError('bootstrap exceeds WinMain extent')
    out = bytearray(data)
    out[start:start + extent] = code
    return bytes(out)
