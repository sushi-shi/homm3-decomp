"""Locate vendor COFF contributions through retail-proven code relocations.

No address bands, nearest-symbol sizes, string searches or zero-run searches.
Archive/object section lengths are accepted only at an anchored retail address
after comparing their bytes and checking the relocation graph.
"""
from __future__ import annotations

from collections import defaultdict, deque
from dataclasses import dataclass
import hashlib
import json
import os
import struct
import subprocess
import sys

from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.core import tsv
from homm3.core.cc_wrap import find_ci
from homm3.core.project import Project


def archive_members(data):
    """Read normal Microsoft COFF members, preserving offsets and long names."""
    if not data.startswith(b'!<arch>\n'):
        raise ValueError('not a COFF archive')
    position, names = 8, b''
    while position < len(data):
        header = data[position:position + 60]
        if len(header) != 60 or header[58:] != b'`\n':
            raise ValueError('invalid archive member header')
        size = int(header[48:58])
        start = position + 60
        if size < 0 or start + size > len(data):
            raise ValueError('truncated archive member')
        name = header[:16].decode('ascii').strip()
        payload = data[start:start + size]
        if name == '//':
            names = payload
        elif name != '/':
            if name.startswith('/') and name[1:].isdigit():
                offset = int(name[1:])
                if offset >= len(names):
                    raise ValueError('invalid archive long name')
                tail = names[offset:]
                name = tail.split(b'\0', 1)[0].split(b'/\n', 1)[0].decode('latin-1')
            else:
                name = name.rstrip('/')
            yield position, name, payload
        position = start + size + (size & 1)


@dataclass
class Object:
    library: str
    member: str
    origin: str
    digest: str
    coff: CoffObject
    unit: str = ''


def load_object(library, member, origin, payload, unit=''):
    return Object(library, member, origin, hashlib.sha256(payload).hexdigest(), CoffObject(payload), unit)


def zlib_key(project):
    paths = [project.root / 'config/units.toml',
             project.root / 'scripts/homm3/core/cc_wrap.py']
    for directory in [*project.includes, project.root / 'vendor/zlib-1.1.3',
                      project.toolchain / 'include', project.toolchain / 'bin']:
        if directory.is_dir():
            paths.extend(p for p in directory.rglob('*') if p.is_file())
    digest = hashlib.sha256(b'vendor-data-zlib-build-v1')
    for path in sorted(set(paths)):
        digest.update(str(path).encode())
        digest.update(hashlib.sha256(path.read_bytes()).digest())
    return digest.hexdigest()


def inputs(root, *, build=False):
    """Read pinned archives and fresh private zlib objects; never trust old base objects."""
    project = Project(root)
    objects, issues, hashes = [], [], {}
    for name in ('LIBCMT.LIB', 'LIBCPMT.LIB'):
        path = find_ci(project.toolchain / 'lib', name)
        if path is None:
            issues.append(dict(kind='missing-library', library=name, detail='pinned toolchain archive unavailable'))
            continue
        payload = path.read_bytes()
        hashes[str(path)] = hashlib.sha256(payload).hexdigest()
        for offset, member, blob in archive_members(payload):
            if blob[:2] != b'\x4c\x01':
                continue  # Linker indexes / short import objects are not storage.
            try:
                objects.append(load_object(name, f'{member}@{offset:x}', str(path), blob))
            except ValueError as exc:
                issues.append(dict(kind='invalid-object', library=name, member=member, detail=str(exc)))
    directory = root / 'build/gen/vendor-data'
    stamp = directory / 'zlib.json'
    key = zlib_key(project)
    units = [u for u in project.manifest['unit'] if u['source'].startswith('vendor/zlib-1.1.3/')]
    try:
        cached = json.loads(stamp.read_text())
    except (OSError, ValueError):
        cached = {}
    fresh = cached.get('fingerprint') == key and all(
        (directory / (u['unit'] + '.obj')).is_file() and
        hashlib.sha256((directory / (u['unit'] + '.obj')).read_bytes()).hexdigest() ==
        cached.get('objects', {}).get(u['unit']) for u in units)
    if build and not fresh:
        directory.mkdir(parents=True, exist_ok=True)
        stamp.unlink(missing_ok=True)
        for u in units:
            output = directory / (u['unit'] + '.obj')
            command = [sys.executable, '-m', 'homm3.core.cc_wrap', '--src', str(root / u['source']),
                       '--out', str(output), '--', *project.manifest['flags'][u['flags']]]
            environment = dict(os.environ, HOMM3_DIR=str(root),
                               PYTHONPATH=str(root / 'scripts') + os.pathsep + os.environ.get('PYTHONPATH', ''))
            result = subprocess.run(command, cwd=root, env=environment, capture_output=True, text=True)
            if result.returncode:
                raise ValueError(f'vendor compilation failed: {u["unit"]}\n{result.stderr}')
        cached = dict(fingerprint=key, objects={u['unit']: hashlib.sha256(
            (directory / (u['unit'] + '.obj')).read_bytes()).hexdigest() for u in units})
        stamp.write_text(json.dumps(cached, indent=2) + '\n')
        fresh = True
    if fresh:
        hashes[str(stamp)] = hashlib.sha256(stamp.read_bytes()).hexdigest()
        for u in units:
            path = directory / (u['unit'] + '.obj')
            payload = path.read_bytes()
            hashes[str(path)] = hashlib.sha256(payload).hexdigest()
            objects.append(load_object('zlib-1.1.3', path.name, str(path), payload, u['unit']))
    else:
        issues.append(dict(kind='missing-or-stale-objects', library='zlib-1.1.3',
                           detail='Run sema coverage --build-vendor to compile fresh pinned VC6 objects.'))
    seeds = defaultdict(set)
    for filename in ('runtime-map.tsv', 'zlib-map.tsv'):
        path = root / 'config/retail' / filename
        hashes[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
        for row in tsv.read(path)[2]:
            seeds[(row.get('unit', ''), row['name'])].add(int(row['rva'], 0))
    return objects, seeds, issues, hashes


def image_bytes(layout, rva, size):
    """Read file backing plus virtual zeros, requiring a single PE section."""
    section = next((s for s in layout.sections if s.rva <= rva < rva + size <= s.rva + s.mapped_size), None)
    if section is None:
        raise ValueError('extent outside one retail section')
    offset = rva - section.rva
    backed = max(0, min(size, section.raw_size - offset))
    return layout.data[section.raw_offset + offset:section.raw_offset + offset + backed] + bytes(size - backed)


def relocation_target(layout, rva, raw, rel):
    """Solve symbol address, retaining COFF addends (including signed REL32)."""
    if rel.typ not in (6, 7, 20) or rel.site + 4 > len(raw):
        raise ValueError(f'unsupported/out-of-bounds relocation {rel.typ:#x} at {rel.site:#x}')
    value = struct.unpack('<I', image_bytes(layout, rva + rel.site, 4))[0]
    addend = struct.unpack_from('<i', raw, rel.site)[0]
    target = (value - layout.base if rel.typ == 6 else value if rel.typ == 7 else
              rva + rel.site + 4 + struct.unpack('<i', struct.pack('<I', value))[0])
    return target - addend


def compare(layout, rva, raw, relocs, *, code=False):
    """Compare every nonrelocation byte; only code alignment NOPs may be trimmed."""
    size = len(raw)
    if code:
        trimmed = min(15, len(raw) - len(raw.rstrip(b'\x90')))
        size -= trimmed
    if not size:
        raise ValueError('empty contribution')
    actual = image_bytes(layout, rva, size)
    mask = bytearray(size)
    for rel in relocs:
        if rel.typ not in (6, 7, 20) or rel.site + 4 > size or any(mask[rel.site:rel.site + 4]):
            raise ValueError('unsupported, overlapping or out-of-bounds relocation')
        mask[rel.site:rel.site + 4] = b'\x01' * 4
    fixed = size - sum(mask)
    if code and fixed < 8:
        raise ValueError('code anchor has fewer than eight fixed bytes')
    if any(a != b for a, b, masked in zip(actual, raw, mask) if not masked):
        raise ValueError('nonrelocation bytes differ from retail')
    return fixed


def match(layout, objects, seeds):
    """Follow matched code/data edges; conflicting placements never erase gaps."""
    nodes, definitions, symbol_nodes = {}, defaultdict(list), {}
    for oi, obj in enumerate(objects):
        coff = obj.coff
        for sec in coff.sections:
            if (not sec.raw_size or not sec.characteristics & 0xe0 or
                    sec.characteristics & 0xa00 or sec.name.startswith('.debug')):
                continue  # Ignore debug/directive/remove sections.
            key = (oi, sec.index)
            nodes[key] = dict(object=obj, section=sec.name, raw=coff.section_bytes(sec),
                              code=bool(sec.characteristics & 0x20), bss=bool(sec.characteristics & 0x80),
                              relocs=[r for r in coff.relocations if r.section == sec.index], symbols=[])
        for sym in coff.symbols.values():
            key = (oi, sym.section)
            offset = sym.value
            if sym.section == 0 and sym.value and sym.storage_class == 2:
                key, offset = (oi, -sym.index - 1), 0
                nodes[key] = dict(object=obj, section='COMMON', raw=bytes(sym.value), code=False,
                                  bss=True, relocs=[], symbols=[])
            elif sym.section <= 0:
                continue  # Absolute/debug/undefined symbols are not contributions.
            if key not in nodes:
                continue
            symbol_nodes[(oi, sym.index)] = (key, offset)
            if sym.storage_class not in (2, 3) or sym.aux_count:
                continue
            nodes[key]['symbols'].append((sym.name, offset))
            if sym.storage_class == 2:
                definitions[sym.name].append((key, offset))

    locations, origins = defaultdict(set), defaultdict(set)
    queue, seen, accepted, rejected, roots = deque(), set(), {}, {}, set()
    symbol_addresses = defaultdict(set)
    for (unit, name), addresses in seeds.items():
        if not unit:
            symbol_addresses[name].update(addresses)
    for key, node in nodes.items():
        if not node['code']:
            continue
        for name, offset in node['symbols']:
            for address in seeds.get((node['object'].unit, name), ()):
                queue.append((key, address - offset, f'admitted function {name} at RVA {address:#x}', None))
                roots.add((key, address - offset))
    edges = defaultdict(list)
    while queue:
        key, rva, evidence, parent = queue.popleft()
        marker = (key, rva)
        origins[marker].add(evidence)
        if parent is not None:
            edges[parent].append(marker)
        if marker in seen:
            continue
        seen.add(marker)
        node = nodes[key]
        try:
            fixed = compare(layout, rva, node['raw'], node['relocs'], code=node['code'])
        except ValueError as exc:
            rejected[marker] = str(exc)
            continue
        locations[key].add(rva)
        accepted[marker] = fixed
        for name, offset in node['symbols']:
            # Only external definitions establish process-wide name addresses.
            if (key, offset) in definitions.get(name, ()):
                symbol_addresses[name].add(rva + offset)
        oi = key[0]
        for rel in node['relocs']:
            sym = node['object'].coff.symbols[rel.symbol_index]
            target = relocation_target(layout, rva, node['raw'], rel)
            targets = ([symbol_nodes[(oi, sym.index)]] if (oi, sym.index) in symbol_nodes else
                       definitions.get(sym.name, []))
            for child, offset in targets:
                queue.append((child, target - offset,
                              f'{node["object"].library}:{node["object"].member}:{node["section"]} '
                              f'RVA {rva + rel.site:#x} -> {sym.name} (addend retained)', marker))

    # A relocation may locate a section, but never resolve contradictory aliases.
    bad = {marker for marker in accepted if len(locations[marker[0]]) != 1}
    checks = {}
    for marker in accepted:
        key, rva = marker
        node = nodes[key]
        unresolved, conflicts = [], []
        for rel in node['relocs']:
            sym = node['object'].coff.symbols[rel.symbol_index]
            actual = relocation_target(layout, rva, node['raw'], rel)
            binding = symbol_nodes.get((key[0], sym.index))
            expected = ({a + binding[1] for a in locations[binding[0]]} if binding else
                        {sym.value - layout.base} if sym.section == -1 else symbol_addresses[sym.name])
            description = f'{rel.site:#x}:{sym.name}'
            if not expected:
                unresolved.append(description)
            elif expected != {actual}:
                conflicts.append(description)
        checks[marker] = (unresolved, conflicts)
        if conflicts:
            bad.add(marker)
    # Every admitted data contribution needs an unbroken path from a surviving
    # named code anchor. A rejected/conflicting parent cannot launder its data
    # into a verified attribution. Pointer-incomplete data does not propagate proof.
    paths = {}
    queue = deque((marker, []) for marker in sorted((roots & accepted.keys()) - bad))
    while queue:
        marker, trail = queue.popleft()
        if marker in paths or marker in bad or marker not in accepted:
            continue
        key, rva = marker
        node = nodes[key]
        trail = [*trail, f'{node["object"].library}:{node["object"].member} '
                 f'section {key[1]} RVA {rva:#x}']
        paths[marker] = trail
        if node['code'] or not checks[marker][0]:
            queue.extend((child, trail) for child in edges[marker])
    rows, issues = [], []
    for marker in sorted(seen):
        key, rva = marker
        node, obj = nodes[key], nodes[key]['object']
        if node['code']:
            continue
        unresolved, conflicts = checks.get(marker, ([], []))
        status = ('rejected' if marker in rejected else 'conflict' if marker in bad else
                  'candidate' if unresolved or marker not in paths else 'verified')
        row = dict(id=len(rows), library=obj.library, member=obj.member, origin=obj.origin,
                   object_sha256=obj.digest, section=node['section'], section_index=key[1],
                   rva=rva, end=rva + len(node['raw']), size=len(node['raw']),
                   storage='bss' if node['bss'] else 'initialized',
                   symbols=[f'{name}+0x{offset:x}' for name, offset in node['symbols']],
                   status=status, fixed_bytes=accepted.get(marker, 0),
                   relocation_count=len(node['relocs']), unresolved_relocations=unresolved,
                   conflicting_relocations=conflicts, evidence=sorted(origins[marker]),
                   anchor_path=paths.get(marker, []),
                   detail=rejected.get(marker, 'multiple/conflicting placements' if marker in bad else
                                       'no surviving verified anchor path' if marker not in paths else ''))
        rows.append(row)
    for marker, detail in sorted(rejected.items()):
        key, rva = marker
        if nodes[key]['code'] and any(e.startswith('admitted function') for e in origins[marker]):
            obj = nodes[key]['object']
            issues.append(dict(kind='unmatched-code-anchor', library=obj.library, member=obj.member,
                               rva=rva, detail=detail))
    libraries = {}
    for library in sorted({o.library for o in objects}):
        own_nodes = {key for key, n in nodes.items() if not n['code'] and n['object'].library == library}
        reached = {key for key, rva in seen} & own_nodes
        libraries[library] = dict(objects=sum(o.library == library for o in objects),
                                 data_contributions=len(own_nodes), reached_contributions=len(reached),
                                 unreached_contributions=len(own_nodes - reached))
    return rows, issues, dict(objects=len(objects), libraries=libraries,
                             code_contributions_matched=sum(nodes[k]['code'] for k, r in accepted),
                             admitted_code_anchors_matched=len(roots & paths.keys()),
                             data_contributions=len(rows), verified_contributions=sum(r['status'] == 'verified' for r in rows))


def extract(root, layout, *, build=False):
    objects, seeds, issues, hashes = inputs(root, build=build)
    rows, findings, summary = match(layout, objects, seeds)
    summary['input_sha256'] = hashes
    summary['inputs_complete'] = not issues
    summary['limitations'] = ['Only relocation-reachable contributions from matched library code are attributed.',
                              'Unmatched, ambiguous or unavailable library objects do not establish absence.',
                              'COFF contribution extents are not individual source-object boundaries.']
    return rows, issues + findings, summary
