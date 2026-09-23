"""Bounded x86 VC6 RTTI/exception records reached through proved roots.

The pinned TYPEINFO header supplies the vptr/cache/name layout. Exception and
class record fields follow the Microsoft ABI (also implemented by LLVM's
MicrosoftCXXABI.cpp). No string scan or nearby-pointer guess creates a root.
"""
from __future__ import annotations

from collections import defaultdict
import hashlib
import struct


from homm3.analysis import vendor_data
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.core.cc_wrap import find_ci
from homm3.core.project import Project


def typeinfo_vtable(root, layout, runtime, functions):
    """Locate the type_info vptr through its admitted, archive-matched destructor."""
    name = '??1type_info@@UAE@XZ'
    entries = runtime.get(name, ())
    if len(entries) != 1:
        raise ValueError('unique admitted type_info destructor required')
    start = next(iter(entries))
    archive = find_ci(Project(root).toolchain/'lib', 'LIBCMT.LIB')
    if archive is None:
        raise ValueError('pinned LIBCMT.LIB is missing')
    payload = archive.read_bytes()
    candidates = set()
    for _, member, data in vendor_data.archive_members(payload):
        if data[:2] != b'\x4c\x01':
            continue
        obj = CoffObject(data)
        for symbol in obj.symbols.values():
            if symbol.name != name or symbol.section <= 0 or symbol.value:
                continue
            section = obj.sections[symbol.section-1]
            raw = obj.section_bytes(section)
            refs = [r for r in obj.relocations if r.section == section.index]
            if functions.get(start) != len(raw):
                continue
            try:
                vendor_data.compare(layout, start, raw, refs, code=True)
            except ValueError:
                continue
            for ref in refs:
                if ref.typ == 6 and obj.symbols[ref.symbol_index].name == '??_7type_info@@6B@':
                    candidates.add(vendor_data.relocation_target(layout, start, raw, ref))
    if len(candidates) != 1:
        raise ValueError('type_info vtable is not uniquely proved by pinned destructor')
    return next(iter(candidates)), dict(path=str(archive), sha256=hashlib.sha256(payload).hexdigest(),
                                       destructor_rva=start)


class Records:
    def __init__(self, layout, typeinfo_vtable):
        self.layout, self.typeinfo_vtable = layout, typeinfo_vtable
        self.rows = {}

    def pointer(self, value, *, code=False, nullable=False):
        if value == 0 and nullable:
            return None
        rva = value-self.layout.base
        if not any(s.rva <= rva < s.rva+s.raw_size and (bool(s.flags & 0x20) if code else
                   s.name in ('.data', '.rdata')) for s in self.layout.sections):
            raise ValueError('record pointer is outside required file-backed storage')
        return rva

    def words(self, rva, count):
        if rva % 4:
            raise ValueError('unaligned compiler record')
        self.pointer(self.layout.base+rva)
        return self.layout.unpack('<'+'I'*count, rva)

    def add(self, rva, size, kind, fields, pointers=()):
        self.layout.read(rva, size)
        record = dict(rva=rva, end=rva+size, size=size, kind=kind, fields=fields,
                      pointers=[dict(offset=offset, target_rva=target) for offset, target in pointers],
                      status='validated', candidate_status='not-compared')
        previous = self.rows.get((kind, rva))
        if previous is not None and previous != record:
            raise ValueError('conflicting compiler record extent or interpretation')
        self.rows[kind, rva] = record

    def descriptor(self, rva):
        vptr, cache = self.words(rva, 2)
        if vptr != self.layout.base+self.typeinfo_vtable or cache:
            raise ValueError('TypeDescriptor vptr/cache does not match pinned type_info')
        size = self.layout.cstring_size(rva+8)
        if not 2 <= size <= 4096:
            raise ValueError('unsupported TypeDescriptor name extent')
        raw = self.layout.read(rva+8, size)
        if raw[0] != ord('.') or any(c < 32 or c > 126 for c in raw[:-1]):
            raise ValueError('invalid TypeDescriptor decorated name')
        self.add(rva, 8+size, 'rtti-type-descriptor', dict(name=raw[:-1].decode('ascii')),
                 [(0, self.typeinfo_vtable)])

    def throw_info(self, rva):
        flags, cleanup, forward, array = self.words(rva, 4)
        if flags & ~7:
            raise ValueError('unsupported VC6 ThrowInfo flags')
        cleanup = self.pointer(cleanup, code=True, nullable=True)
        forward = self.pointer(forward, code=True, nullable=True)
        array = self.pointer(array)
        count, = self.words(array, 1)
        if not 1 <= count <= 1024:
            raise ValueError('invalid catchable-type count')
        types = self.words(array+4, count)
        pointers = []
        for index, value in enumerate(types):
            target = self.pointer(value)
            flags_, descriptor, mdisp, pdisp, vdisp, size, copy = self.words(target, 7)
            if flags_ & ~7 or not size or size > self.layout.image_size:
                raise ValueError('unsupported catchable-type flags or size')
            descriptor = self.pointer(descriptor)
            copy = self.pointer(copy, code=True, nullable=True)
            self.descriptor(descriptor)
            self.add(target, 28, 'eh-catchable-type', dict(flags=flags_, size=size,
                     member_displacement=mdisp, vbptr_displacement=pdisp, vbtable_displacement=vdisp),
                     [(4, descriptor), (24, copy)])
            pointers.append((4+index*4, target))
        self.add(array, 4+count*4, 'eh-catchable-type-array', dict(count=count), pointers)
        self.add(rva, 16, 'eh-throw-info', dict(flags=flags), [(4, cleanup), (8, forward), (12, array)])

    def locator(self, rva):
        signature, offset, cd_offset, descriptor, hierarchy = self.words(rva, 5)
        if signature:
            raise ValueError('unsupported x86 CompleteObjectLocator signature')
        descriptor, hierarchy = self.pointer(descriptor), self.pointer(hierarchy)
        self.descriptor(descriptor)
        signature_, attributes, count, array = self.words(hierarchy, 4)
        if signature_ or attributes & ~7 or not 1 <= count <= 1024:
            raise ValueError('unsupported class hierarchy header')
        array = self.pointer(array)
        pointers = []
        for index, pointer in enumerate(self.words(array, count)):
            target = self.pointer(pointer)
            type_, contained, mdisp, pdisp, vdisp, flags = self.words(target, 6)
            # VC6's descriptor is 24 bytes. The newer optional pClassDescriptor
            # extension is deliberately not guessed from adjacent words.
            if flags & ~0x3f or contained >= count:
                raise ValueError('unsupported base-class descriptor extent/flags')
            type_ = self.pointer(type_)
            self.descriptor(type_)
            self.add(target, 24, 'rtti-base-class-descriptor', dict(contained_bases=contained,
                     member_displacement=mdisp, vbptr_displacement=pdisp, vbtable_displacement=vdisp,
                     attributes=flags), [(0, type_)])
            pointers.append((index*4, target))
        self.add(array, count*4, 'rtti-base-class-array', dict(count=count), pointers)
        self.add(hierarchy, 16, 'rtti-class-hierarchy', dict(attributes=attributes, count=count), [(12, array)])
        self.add(rva, 20, 'rtti-complete-object-locator', dict(offset=offset, constructor_displacement=cd_offset),
                 [(12, descriptor), (16, hierarchy)])


def throw_roots(layout, functions, throw_entries):
    """Only reachable direct calls with a dataflow-proved second argument."""
    from homm3.analysis import data_effects
    roots, issues = [], []
    targets = {layout.base+rva for rva in throw_entries}
    for start, size in functions.items():
        raw = layout.read(start, size)
        # Cheap opcode prefilter; decoded reachable call/argument evidence below
        # is the admission rule, not this byte search.
        if not any(raw[i] == 0xe8 and start+i+5+struct.unpack_from('<i', raw, i+1)[0] in throw_entries
                   for i in range(max(0, len(raw)-4))):
            continue
        for event in data_effects.analyze(raw, layout.base+start)['events']:
            target = event.get('target')
            if event['kind'] != 'call' or target is None or target[0] != 'integer' or target[1] not in targets:
                continue
            argument = event['arguments'][1]
            if argument is None or argument[0] != 'integer':
                issues.append(dict(kind='throw-argument-unresolved', function_rva=start, site_rva=event['site']-layout.base))
                continue
            value = argument[1]
            if value == 0:
                continue  # rethrow has no new ThrowInfo
            roots.append(dict(kind='throw', rva=value-layout.base, function_rva=start,
                              site_rva=event['site']-layout.base, evidence='reachable direct __CxxThrowException@8 call; dataflow-proved second argument'))
    return roots, issues


def collect(root, layout, functions, runtime, claims):
    vtable, proof = typeinfo_vtable(root, layout, runtime, functions)
    roots, issues = throw_roots(layout, functions, runtime.get('__CxxThrowException@8', ()))
    for claim in claims:
        if claim['kind'] != 'eh-catch-map' or claim['confidence'] != 'proven':
            continue
        for offset in range(0, claim['size'], 16):
            slot = claim['start']+offset+4
            pointer, = layout.unpack('<I', slot)
            if pointer:
                roots.append(dict(kind='catch', rva=pointer-layout.base, site_rva=slot,
                                  evidence='TypeDescriptor pointer in validated EH catch map'))
    from homm3.sema.retail_claims import rows as legacy_rows
    vtables = {int(r['rva'], 0) for r in legacy_rows(root/'config/retail/vtables.tsv')} | {vtable}
    absent = 0
    for address in sorted(vtables):
        try:
            pointer, = layout.unpack('<I', address-4)
            target = pointer-layout.base
            section = next((s for s in layout.sections if s.name in ('.data', '.rdata') and
                            s.rva <= target < s.rva+s.raw_size), None)
            if section is None or layout.unpack('<I', target)[0] != 0:
                absent += 1
                continue
        except ValueError:
            absent += 1
            continue
        roots.append(dict(kind='locator', rva=target, site_rva=address-4,
                          evidence='locator pointer preceding admitted vtable (or archive-proved type_info vtable)'))
    rows, links = {}, defaultdict(list)
    for index, row in enumerate(roots):
        row['id'] = index
        parser = Records(layout, vtable)
        try:
            if row['kind'] == 'throw':
                parser.throw_info(row['rva'])
            elif row['kind'] == 'catch':
                parser.descriptor(row['rva'])
            else:
                parser.locator(row['rva'])
                parser.add(row['site_rva'], 4, 'rtti-vtable-locator-slot', {}, [(0, row['rva'])])
        except ValueError as exc:
            row.update(status='invalid', detail=str(exc))
            issues.append(dict(kind='invalid-rtti-root', root_id=index, rva=row['rva'], detail=str(exc)))
            continue  # Commit this root's records only after the entire graph validates.
        row.update(status='validated', detail='')
        for key, record in parser.rows.items():
            if key in rows and rows[key] != record:
                raise ValueError('inconsistent RTTI record between validated roots')
            rows[key] = record
            links[key].append(index)
    structures = [dict(row, id=index, root_ids=links[key])
                  for index, (key, row) in enumerate(sorted(rows.items(), key=lambda pair: (pair[1]['rva'], pair[0][0])))]
    intervals = sorted((r['rva'], r['end']) for r in structures)
    size, end = 0, 0
    for lo, hi in intervals:
        size += max(0, hi-max(lo, end))
        end = max(end, hi)
    return dict(structures=structures, roots=roots, issues=issues, proof=proof,
                summary=dict(structures=len(structures), distinct_structure_bytes=size,
                             roots=len(roots), invalid_roots=sum(r['status'] != 'validated' for r in roots),
                             vtables_without_locator=absent, unresolved_throw_arguments=sum(
                                 i['kind'] == 'throw-argument-unresolved' for i in issues),
                             typeinfo_vtable_rva=vtable))
