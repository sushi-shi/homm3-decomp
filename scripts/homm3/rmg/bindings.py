"""COFF absolute-symbol bridges; no runtime code is synthesized here."""
from __future__ import annotations

import csv
import re
import struct
from pathlib import Path

from homm3.build.canonicalize_data_symbols import CoffObject

DRIVER_BASE = 0x30000000

# These are external services, never an escape hatch for missing RMG bodies.
SERVICES = {
    '??0TGzFile@@QAE@PBD0@Z', '??1TGzFile@@UAE@XZ',
    '??0TRuntimeError@@QAE@PBD@Z',
    '?buildTileNeighbourMask@@YIXHHHHPAE@Z',
    '?getImageName@TObjectType@@QAEABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ',
    '?getSpreadsheet@ResourceManager@@YIPAVTSpreadsheetResource@@PBD@Z',
    '?load@TObjectTypeTable@@QAEXPAD@Z',
}
DATA_SERVICES = {
    'g_adventureObjectLandBlocked', 'g_artifactTraits',
    'g_creatureGenerator1Types', 'g_creatureTypeTraits', 'g_heroTraits',
    'g_spellTraits', 'g_tileDirections',
}
RUNTIME = {
    '??0_Lockit@std@@QAE@XZ', '??1_Lockit@std@@QAE@XZ',
    '??0exception@@QAE@ABQBD@Z', '??0exception@@QAE@ABV0@@Z',
    '??1exception@@UAE@XZ', '??2@YAPAXI@Z', '??3@YAXPAX@Z',
    '??_L@YGXPAXIHP6EX0@Z1@Z', '??_M@YGXPAXIHP6EX0@Z@Z',
    '?_Xlen@std@@YAXXZ', '?_Xran@std@@YAXXZ', '__CxxThrowException@8',
    '___CxxFrameHandler', '__chkstk', '__ftol', '__purecall', '_atexit',
    '_atoi', '_memmove', '_rand', '_sprintf', '_sqrt', '_srand', '_time', '_tolower',
}
ALIASES = {
    '??1_Lockit@std@@QAE@XZ': 'exe_scoped_lock_exe_scoped_lock',
    '_srand': 'exe_srand', '__purecall': 'exe_purecall',
    '__chkstk': '__alloca_probe',
    # Retail string-copy constructor 0x4044e0 calls these at +0x64/+0x8c.
    '?_Xran@std@@YAXXZ': 'cxx_invalid_string_position_20ac23',
    '_memmove': 'crt_44e0_sub04_217590',
}


def data_addresses(root: Path) -> dict[str, int]:
    """Read the owning DATA declarations, including pointer/reference slots."""
    from homm3.retail_labels.source import mask_lexical_noise, macro_invocations, DATA_HEAD_RE
    found: dict[str, set[int]] = {}
    for directory in ('include', 'src'):
        for path in sorted((root / directory).glob('*')):
            if path.suffix not in ('.h', '.cpp', '.c'):
                continue
            raw = path.read_text()
            masked = mask_lexical_noise(raw)
            for _, end, args, _ in macro_invocations(masked, DATA_HEAD_RE, raw):
                if end is None or len(args) != 1:
                    continue
                # Stop before an initializer, body, or next declaration.
                declaration = re.split(r'[;={]', masked[end:], maxsplit=1)[0]
                for name in DATA_SERVICES:
                    if re.search(r'\b' + name + r'\b', declaration):
                        found.setdefault(name, set()).add(int(args[0], 16))
    if set(found) != DATA_SERVICES or any(len(v) != 1 for v in found.values()):
        raise ValueError(f'missing or ambiguous external DATA annotations: {found}')
    return {name: next(iter(values)) for name, values in found.items()}


def resolve(root: Path, paths: list[Path], retail: bytes) -> dict[str, int]:
    from .bootstrap import offset
    rows = list(csv.DictReader(line for line in
                (root / 'build/gen/symbol_names.csv').read_text().splitlines()
                if not line.startswith('#')))
    inventory = {row['name']: row for row in rows}
    data = data_addresses(root)
    result = {}
    for name in sorted(undefined_symbols(paths)):
        if name.startswith('__imp__'):
            continue  # Resolved solely by the explicit Windows import library.
        if name in ('__except_list', '__fltused'):
            result[name] = 0  # fs:[0] offset and unused linker presence marker.
        elif name == '__except_handler3':
            # Real CRT entry 0x61a2b4 pushes this handler before __cinit.
            result[name] = 0x61a528
        elif name == '??_7type_info@@6B@':
            # Retail exception RTTI type descriptor: vptr, cache, name.
            marker = b'.?AVexception@@\0'
            start = retail.index(marker)
            if retail.find(marker, start + 1) != -1:
                raise ValueError('ambiguous retail exception type descriptor')
            result[name], = struct.unpack_from('<I', retail, start - 8)
            offset(retail, result[name])
        elif name in SERVICES or name in RUNTIME:
            row = inventory[ALIASES.get(name, name)]
            if row['unit'] in ('rmg', 'rmg_support', 'rmg_terrain'):
                raise ValueError(f'forbidden retail RMG binding: {name}')
            result[name] = int(row['rva'], 16) + 0x400000
        else:
            match = re.match(r'^\?(g_\w+)@@', name)
            if not match or match[1] not in DATA_SERVICES:
                raise ValueError(f'unresolved candidate dependency (no retail fallback): {name}')
            result[name] = data[match[1]]
    return result


def prepare_calls(payload: bytes, bindings: dict[str, int]) -> bytes:
    """Compensate VC6 LINK's REL32 arithmetic for absolute externals.

    LINK subtracts the site's RVA for ABS symbols (DIR32 correctly uses the
    absolute value). Subtract the fixed DLL base in the REL32 addend only.
    This disposable link input changes no instruction or source object.
    """
    obj = CoffObject(payload)
    out = bytearray(payload)
    for relocation in obj.relocations:
        if relocation.typ != 0x14 or obj.symbols[relocation.symbol_index].name not in bindings:
            continue
        section = obj.sections[relocation.section - 1]
        site = section.raw_offset + relocation.site
        value, = struct.unpack_from('<I', out, site)
        struct.pack_into('<I', out, site, (value - DRIVER_BASE) & 0xffffffff)
    return bytes(out)


def absolute_object(bindings: dict[str, int]) -> bytes:
    strings = bytearray(b'\0\0\0\0')
    symbols = bytearray()
    for name, address in sorted(bindings.items()):
        encoded = name.encode('ascii')
        if len(encoded) <= 8:
            field = encoded.ljust(8, b'\0')
        else:
            field = struct.pack('<II', 0, len(strings))
            strings.extend(encoded + b'\0')
        symbols.extend(field + struct.pack('<IhHBB', address, -1, 0, 2, 0))
    struct.pack_into('<I', strings, 0, len(strings))
    return struct.pack('<HHIIIHH', 0x14c, 0, 0, 20, len(bindings), 0, 0) + symbols + strings


def undefined_symbols(paths: list[Path]) -> set[str]:
    objects = [CoffObject(path.read_bytes()) for path in paths]
    defined = {s.name for obj in objects for s in obj.symbols.values()
               if s.storage_class == 2 and (s.section != 0 or s.value != 0)}
    # VC6 also emits weak external aliases, resolved by LINK from aux records.
    aliases = {s.name for obj in objects for s in obj.symbols.values()
               if s.storage_class == 105}
    return {s.name for obj in objects for s in obj.symbols.values()
            if s.storage_class == 2 and s.section == 0} - defined - aliases
