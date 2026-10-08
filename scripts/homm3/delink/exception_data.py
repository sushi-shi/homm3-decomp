"""Compare exception records emitted by the project's ordinary VC6 sources.

A type descriptor's complete encoded type name is its identity anchor.
Catchable types, arrays and throw records extend that graph only through
already identified referents. All bytes, pointer addends and relocation sites
must agree; unknown constructors/destructors are never masked away.
"""
from collections import defaultdict
from dataclasses import dataclass
from hashlib import sha256
import struct

from homm3.core import compile_receipt
from homm3.delink import coffx
from homm3.core.images import path as _image_path


@dataclass(frozen=True)
class Record:
    name: str
    unit: str
    payload: bytes
    relocations: tuple
    alignment: int


def supported(record):
    name, body = record.name, record.payload
    rel = {off: (target, kind) for off, target, kind in record.relocations}
    if any(kind != 6 or off % 4 or off < 0 or off+4 > len(body)
           for off, _, kind in record.relocations):
        return False
    if name.startswith('??_R0') and name.endswith('@8'):
        encoded = b'.' + name[5:-2].encode('ascii') + b'\0'
        return (len(body) == 8+len(encoded) and body[4:8] == bytes(4)
                and body[8:] == encoded and rel == {0: ('??_7type_info@@6B@', 6)})
    if name.startswith('__CTA'):
        if len(body) < 8:
            return False
        count = struct.unpack_from('<I', body)[0]
        return (len(body) == 4*(count+1) and set(rel) == set(range(4, len(body), 4))
                and all(target.startswith('__CT??_R0') for target, _ in rel.values()))
    if name.startswith('__CT??_R0'):
        return (len(body) == 28 and set(rel) in ({4}, {4, 24})
                and rel[4][0].startswith('??_R0'))
    if name.startswith('__TI'):
        return (len(body) == 16 and 12 in rel and rel[12][0].startswith('__CTA')
                and set(rel) <= {4, 8, 12})
    return False


_CANDIDATES: dict = {}


def _objects_signature(base_dir) -> tuple:
    """Identity of the raw objects and their content receipts, by stat."""
    from pathlib import Path
    signature = []
    for path in sorted(Path(base_dir).glob('*.obj*')):
        try:
            stat = path.stat()
        except OSError:
            continue
        signature.append((path.name, stat.st_size, stat.st_mtime_ns))
    return tuple(signature)


def candidates(project, base_dir, *, witnesses=None):
    """Only raw objects with current source/compiler content receipts qualify.

    One delink resolves the model and its exception identities several times
    over the same objects; the scan is reused while the objects, their
    receipts and the project root are unchanged within this process.
    """
    from pathlib import Path
    key = (str(Path(project.root).resolve()), str(Path(base_dir).resolve()),
           _objects_signature(base_dir))
    cached = _CANDIDATES.get(key)
    if cached is None:
        found = {}
        records, withheld = _candidates(project, base_dir, witnesses=found)
        _CANDIDATES.clear()
        cached = _CANDIDATES[key] = (records, withheld, found)
    records, withheld, found = cached
    if witnesses is not None:
        witnesses.update(found)
    return list(records), list(withheld)


def _candidates(project, base_dir, *, witnesses=None):
    from homm3.core.cc_wrap import scan_header_deps
    by_unit = {u['unit']: u for u in project.manifest['unit']}
    records, withheld, hashes, headers = [], [], {}, {}
    compiler_files = [p for p in (project.toolchain / 'bin').iterdir()
                      if p.is_file() and p.suffix.lower() in ('.exe', '.dll')]
    for unit, obj in coffx.objects(base_dir):
        emitted = []
        for sec in obj.section_table:
            members = obj.defined_symbols(sec['index'])
            if len(members) != 1 or members[0][0] != 0:
                continue
            name = members[0][1]
            if not name.startswith(('??_R0', '__CT', '__TI')):
                continue
            record = Record(name, unit, obj.section_payload(sec['index'])[:sec['size']],
                            tuple(sorted((off, target, kind) for off, (target, kind)
                                         in obj.typed_relocations(sec['index']).items())),
                            sec['alignment'])
            if len(record.payload) == sec['size'] and supported(record):
                emitted.append(record)
        if not emitted:
            continue
        row = by_unit.get(unit)
        inputs = None
        if row:
            source = project.root / row['source']
            required = [source, project.root / _image_path('config/units.toml'),
                        project.root / 'config/project.toml', *compiler_files,
                        *scan_header_deps(source, project.toolchain / 'include',
                                          *project.includes, cache=headers)]
            inputs = compile_receipt.current(base_dir / f'{unit}.obj',
                flags=project.manifest['flags'][row['flags']], required=required, hashes=hashes)
        if inputs is None or compile_receipt.digest(base_dir / f'{unit}.obj') != sha256(obj.buf).hexdigest():
            withheld.append((0, unit, 'exception metadata has no current source object'))
        else:
            records.extend(emitted)
            if witnesses is not None:
                witnesses[unit] = (obj, inputs)
    return records, withheld


def relocated(record, known, image_base):
    expected = bytearray(record.payload)
    for off, name, _kind in record.relocations:
        if len(known.get(name, ())) != 1:
            return None
        addend = struct.unpack_from('<i', expected, off)[0]
        target = next(iter(known[name])) + image_base + addend
        if not 0 <= target <= 0xffffffff:
            return None
        struct.pack_into('<I', expected, off, target)
    return bytes(expected)


def resolve(records, image, known):
    """Return complete matches, rejecting ambiguous names and destinations."""
    grouped = defaultdict(list)
    for record in records:
        grouped[record.name].append(record)
    withheld, active = [], {}
    for name, copies in grouped.items():
        if len({(r.payload, r.relocations) for r in copies}) != 1:
            withheld.append((0, name, 'exception COMDAT copies disagree'))
        else:
            active[name] = copies[0]
    known = {name: set(addresses) for name, addresses in known.items()}
    reverse = defaultdict(set)
    for site in image.reloc_sites:
        reverse[image.u32(site)].add(site)
    placed = {}
    while active:
        changed = False
        for name, record in list(active.items()):
            expected = relocated(record, known, image.image_base)
            if expected is None:
                continue
            # The complete relocation target set bounds the search. R0 is
            # additionally self-identifying through its full encoded name.
            addresses = None
            for off, _, _ in record.relocations:
                target = struct.unpack_from('<I', expected, off)[0]
                possible = {site-off for site in reverse[target]}
                addresses = possible if addresses is None else addresses & possible
            matches = []
            for rva in addresses or ():
                if (not any(s['name'] in ('.data', '.rdata') and s['va'] <= rva
                            and rva+len(expected) <= s['va']+s['rsize']
                            for s in image.pe.sections)
                        or rva % record.alignment):
                    continue
                sites = {rva+off for off, _, _ in record.relocations}
                if (image.pe.read(rva, len(expected)) == expected
                        and set(image.relocs_in(rva, rva+len(expected))) == sites):
                    matches.append(rva)
            if len(matches) == 1 and (name not in known or known[name] == set(matches)):
                placed[name] = matches[0]
                known[name] = set(matches)
                del active[name]
                changed = True
            elif matches:
                withheld.append((matches[0], name, 'exception record address ambiguous or contradicts model'))
                del active[name]
        if not changed:
            break
    for name, record in active.items():
        missing = [target for _, target, _ in record.relocations
                   if len(known.get(target, ())) != 1]
        withheld.append((0, name, 'exception record unresolved: ' + ', '.join(missing)
                         if missing else 'exception record bytes or relocations differ'))
    rows = [(r, placed[name]) for name, copies in grouped.items() if name in placed
            for r in copies]
    # Two distinct compiler identities cannot claim overlapping storage.
    unique = sorted({(rva, rva+len(r.payload), r.name) for r, rva in rows})
    conflicts = set()
    for i, (start, end, name) in enumerate(unique):
        for other_start, other_end, other in unique[i+1:]:
            if other_start >= end:
                break
            conflicts.update((name, other))
    withheld.extend((placed[n], n, 'exception record overlaps another compiler identity')
                    for n in sorted(conflicts))
    while True:
        dependent = {r.name for r, _ in rows if any(target in conflicts
                     for _, target, _ in r.relocations)} - conflicts
        if not dependent:
            break
        withheld.extend((placed[n], n, 'exception record refers to a conflicting identity')
                        for n in sorted(dependent))
        conflicts.update(dependent)
    return [(r, rva) for r, rva in rows if r.name not in conflicts], withheld


def rows(model, base_dir, existing=()):
    from homm3.core.common import HOMM3_DIR
    from homm3.core.project import Project
    from homm3.delink.image import retail
    from homm3.delink.data_manifest import STORAGE, _classify
    records, withheld = candidates(Project(HOMM3_DIR), base_dir)
    known = defaultdict(set)
    for b in model.data + model.functions:
        if b.name and b.channel:
            known[b.name].add(b.rva)
        for alias in b.aliases:
            if alias.name and alias.channel:
                known[alias.name].add(b.rva)
    for row in existing:
        known[row['name']].add(row['rva'])
    matched, unavailable = resolve(records, retail(), known)
    withheld.extend(unavailable)
    result = []
    for record, rva in matched:
        storage = _classify(rva)
        if storage not in STORAGE or _classify(rva+len(record.payload)-1) != storage:
            withheld.append((rva, record.name, 'exception record crosses storage boundary'))
            continue
        result.append(dict(name=record.name, object=f'{record.unit}.c', rva=rva,
                           size=len(record.payload), storage=STORAGE[storage],
                           alignment=record.alignment, section_placed=True,
                           provenance='candidate-COFF-exception-exact'))
    return result, withheld
