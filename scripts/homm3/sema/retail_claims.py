"""Evidence layers for whole-retail accounting; labels never establish extents."""
from __future__ import annotations

from collections import defaultdict
import csv
import re
import struct
import tomllib

from homm3.sema.coverage import system_claims


def rows(path, delimiter='\t'):
    with path.open() as stream:
        return list(csv.DictReader((line for line in stream if line.strip() and
                                    not line.startswith('#')), delimiter=delimiter))


def claim(start, size, kind, evidence, owner='', *, confidence='proven', domain='image', shared_group=''):
    return dict(domain=domain, start=start, size=size, kind=kind, confidence=confidence,
                evidence=evidence, owner=owner, shared_group=shared_group)


def resource_claims(layout):
    base, size = layout.directory(2)
    if not base or not size:
        return []
    layout.read(base, size)
    result = []
    def bounded(offset, count):
        if offset < 0 or offset + count > size:
            raise ValueError('resource metadata outside resource directory')
        return layout.read(base + offset, count)
    def add(offset, count, kind, path):
        bounded(offset, count)
        result.append(claim(base + offset, count, kind, 'PE resource directory walk', path))
    def walk(offset, path, stack):
        if offset in stack or len(stack) > 16:
            raise ValueError('cyclic/deep resource directory')
        header = bounded(offset, 16)
        named, ids = struct.unpack_from('<HH', header, 12)
        add(offset, 16, 'resource-directory', path or 'resource root')
        bounded(offset + 16, 8 * (named + ids))
        for index in range(named + ids):
            entry = offset + 16 + index * 8
            name, child = struct.unpack('<II', bounded(entry, 8))
            if name & 0x80000000:
                name_offset = name & 0x7fffffff
                length = struct.unpack('<H', bounded(name_offset, 2))[0]
                text = bounded(name_offset + 2, length * 2).decode('utf-16le')
                add(name_offset, 2 + length * 2, 'resource-name', text)
            else:
                text = str(name)
            full = f'{path}/{text}'
            add(entry, 8, 'resource-entry', full)
            child_offset = child & 0x7fffffff
            if child & 0x80000000:
                walk(child_offset, full, stack | {offset})
            else:
                payload, count, _codepage, _reserved = struct.unpack('<4I', bounded(child_offset, 16))
                add(child_offset, 16, 'resource-data-entry', full)
                if count:
                    layout.read(payload, count)
                    result.append(claim(payload, count, 'resource-payload',
                                        'PE resource data entry', full))
    walk(0, '', set())
    return result


def eh_claims(layout, image, root):
    """Require record validation AND a stub jumping to the admitted runtime."""
    from homm3.vc6 import tryblocks
    runtime = rows(root / 'config/retail/runtime-map.tsv')
    handlers = {int(r['rva'], 0) for r in runtime if r['name'] == '___CxxFrameHandler'}
    if not handlers or not {'.text', '.rdata'} <= {s.name for s in layout.sections}:
        return []
    infos = tryblocks.func_infos(image)
    stubs = tryblocks.handler_stubs(image, infos)
    result = []
    for info in infos:
        start = info['info_rva']
        stub = stubs.get(start)
        if stub is None:
            continue
        displacement = layout.unpack('<i', stub + 6)[0]
        if stub + 10 + displacement not in handlers:
            continue
        owner = f'EH FuncInfo RVA 0x{start:x}, handler RVA 0x{stub:x}'
        def add(rva, size, kind):
            layout.read(rva, size)
            result.append(claim(rva, size, kind,
                                'validated VC6 record + stub to admitted ___CxxFrameHandler', owner))
        add(start, tryblocks._FUNCINFO_CB, 'eh-funcinfo')
        add(stub, 10, 'eh-handler-stub')
        unwind = layout.unpack('<I', start + 8)[0] - layout.base
        if info['max_state']:
            add(unwind, 8 * info['max_state'], 'eh-unwind-map')
        tries = layout.unpack('<I', start + 16)[0] - layout.base
        if info['n_try']:
            add(tries, 20 * info['n_try'], 'eh-try-map')
        for index in range(info['n_try']):
            count, handlers_va = layout.unpack('<II', tries + 20 * index + 12)
            if count:
                add(handlers_va - layout.base, 16 * count, 'eh-catch-map')
    return result


def collect(root, layout, image):
    """Return claims, point labels, hashed-input paths and explicit limitations."""
    claims, labels, inputs, limitations = [], defaultdict(list), [], []
    def read(relative, delimiter='\t', optional=False):
        path = root / relative
        if optional and not path.is_file():
            limitations.append(f'optional navigation absent: {relative}')
            return []
        inputs.append(relative)
        return rows(path, delimiter)
    units = tomllib.loads((root / 'config/units.toml').read_text())
    inputs.append('config/units.toml')
    unit_sources = {u['unit']: u['source'] for u in units['unit']}
    for row in read('build/gen/symbol_names.csv', ',', optional=True):
        labels[int(row['rva'], 0)].append(dict(name=row['name'],
            source=unit_sources.get(row['unit'], row['unit']),
            evidence='build/gen/symbol_names.csv: ' + row['provenance']))
    # Preserve all per-TU aliases before the model's one-row-per-RVA join.
    fragments = sorted((root / 'build/gen/claims').glob('*.tsv'))
    if not fragments:
        limitations.append('optional per-TU claim fragments absent; generated model aliases may be incomplete')
    for path in fragments:
        relative = str(path.relative_to(root))
        for row in read(relative):
            labels[int(row['rva'], 0)].append(dict(name=row['name'],
                source=unit_sources.get(path.stem, path.stem), evidence=relative + ': ' + row['channel']))
    for rva in labels:
        labels[rva] = list({(r['name'], r['source'], r['evidence']): r for r in labels[rva]}.values())
    functions = read('config/retail/functions.tsv')
    for row in functions:
        rva, size = int(row['rva'], 0), int(row['size'], 0)
        claims.append(claim(rva, size, 'function', 'config/retail/functions.tsv (includes embedded tables)',
                            f'function RVA 0x{rva:x}'))
    for row in read('config/retail/vtables.tsv'):
        rva = int(row['rva'], 0)
        claims.append(claim(rva, int(row['function_count']) * 4, 'vtable',
                            'config/retail/vtables.tsv', row.get('class') or f'vtable RVA 0x{rva:x}'))
    for row in read('config/retail/data-extents.tsv'):
        if row['category'] not in ('object', 'system', 'padding', 'provisional') or not row['evidence'].strip():
            raise ValueError('reviewed extent needs a valid category and evidence')
        claims.append(claim(int(row['rva'], 0), int(row['size'], 0), row['category'], row['evidence'],
                            confidence='provisional' if row['category'] == 'provisional' else 'proven',
                            shared_group=row.get('shared_group', '')))
    # Header structures are not one broad claim that hides header slack.
    for start, end, kind in [(0, 64, 'dos-header'), (64, layout.pe, 'dos-stub'),
                             (layout.pe, layout.pe + 4, 'pe-signature'),
                             (layout.pe + 4, layout.optional, 'coff-header'),
                             (layout.optional, layout.section_table, 'optional-header'),
                             (layout.section_table, layout.section_table_end, 'section-table')]:
        if end > start:
            claims.append(claim(start, end - start, kind, 'PE header boundaries', kind))
    sections = [dict(name=s.name, rva=s.rva, raw_size=s.raw_size, raw_offset=s.raw_offset)
                for s in layout.sections]
    for c in system_claims(layout.data, sections, layout.optional):
        claims.append(claim(c['rva'], c['size'], 'import-structure', c['evidence'], c['evidence']))
    claims.extend(resource_claims(layout))
    inputs.append('config/retail/runtime-map.tsv')
    claims.extend(eh_claims(layout, image, root))
    # Recognized strings are bounded byte observations, not original object proof.
    for section in layout.sections:
        if section.name not in ('.rdata', '.data'):
            continue
        payload = layout.data[section.raw_offset:section.raw_offset + section.raw_size]
        for match in re.finditer(rb'[\x09\x0a\x0d\x20-\x7e]{4,}\x00', payload):
            claims.append(claim(section.rva + match.start(), len(match.group()), 'string-candidate',
                                'printable NUL-terminated run; original extent/identity unproved',
                                match.group()[:-1].decode('ascii'), confidence='provisional'))
    return claims, labels, inputs, limitations
