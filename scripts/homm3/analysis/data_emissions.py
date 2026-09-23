"""Recognize independently compiled evidence copies of one admitted TU.

Equal initializer bytes never establish identity. A private vendor compilation
must name the same manifest unit, have identical fresh compiler inputs, and keep
the same COFF layout. Both copies remain enrolled and byte-compared.
"""
from collections import defaultdict
import hashlib
import json
from pathlib import Path

from homm3.analysis import candidate_data
from homm3.build import compiled_freshness
from homm3.core.project import Project


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def topology(obj):
    """COFF ownership/relocation structure; never code or initializer values."""
    optional_end = 20+int.from_bytes(obj.data[16:18], 'little')
    return dict(header=obj.data[:4].hex()+obj.data[16:optional_end].hex(),
        sections=[dict(index=s.index, name=s.name, size=s.raw_size, flags=s.characteristics,
                       address_fields=obj.data[s.header_offset+8:s.header_offset+16].hex(),
                       directives=obj.section_bytes(s).hex() if s.name == '.drectve' else '')
                  for s in obj.sections],
        symbols=[dict(index=s.index, name=s.name, value=s.value, section=s.section, type=s.typ,
                      storage=s.storage_class, aux=obj.data[s.offset+18:s.offset+18*(1+s.aux_count)].hex())
                 for s in obj.symbols.values()],
        relocations=[(r.section, r.site, r.typ, r.symbol_index) for r in obj.relocations])


def compiler_inputs(record, obj):
    record = dict(record)
    if (record.get('schema') != compiled_freshness.SCHEMA or
            not {'source', 'flags', 'includes', 'toolchain', 'files', 'trees'} <= record.keys()):
        raise ValueError('emission copy has incomplete compiler provenance')
    if record.pop('object_sha256', '') != hashlib.sha256(obj.data).hexdigest():
        raise ValueError('emission copy disagrees with raw compiler provenance')
    return record


def prove(unit, source_path, original, original_record, copied, copied_record):
    a, b = compiler_inputs(original_record, original), compiler_inputs(copied_record, copied)
    result = dict(source_unit=unit, source=str(source_path),
                  source_object_sha256=hashlib.sha256(original.data).hexdigest(),
                  copy_object_sha256=hashlib.sha256(copied.data).hexdigest(),
                  status='different-compiler-inputs', identity='', allocation_pairs=[])
    if a != b or a.get('source') != str(source_path):
        return result
    result['inputs_sha256'] = digest(a)
    left, right = topology(original), topology(copied)
    if left != right:
        result['status'] = 'different-coff-layout'
        return result
    result['topology_sha256'] = digest(left)
    result['identity'] = 'emission:'+digest([unit, a, left])
    result['status'] = 'same-manifest-emission'
    return result


def identify(root, source_objects, vendor_objects):
    """Source objects have already passed candidate_data's freshness checks."""
    manifest = {u['unit']: u for u in Project(root).manifest['unit']}
    records = []
    for copied_unit, copied in sorted(vendor_objects.items()):
        unit = copied.unit
        # Archive members have no manifest TU identity; this cannot coalesce
        # arbitrary Microsoft library alternatives or identical constants.
        if not unit or unit not in source_objects or unit not in manifest:
            continue
        source_path = (root/manifest[unit]['source']).resolve()
        original_path = root/f'build/objdiff/base/{unit}.obj'
        copied_path = Path(copied.origin)
        paths = [compiled_freshness.stamp_path(p) for p in (original_path, copied_path)]
        stamps = [json.loads(p.read_text()) for p in paths]
        original = source_objects[unit]
        proof = prove(unit, source_path, original, stamps[0], copied.coff, stamps[1])
        proof.update(copy_unit=copied_unit, source_stamp=str(paths[0]), copy_stamp=str(paths[1]))
        if proof['status'] == 'same-manifest-emission':
            # Recheck both on-disk outputs against the already verified source
            # inputs. Header/profile/compiler changes cannot bless an old copy.
            expected = compiler_inputs(stamps[0], original)
            for path in (original_path, copied_path):
                compiled_freshness.validate(path, expected)
            rows = candidate_data.inventory(unit, original, proof['source_object_sha256'])
            for row in rows:
                suffix = row['id'][len(unit)+1:]
                proof['allocation_pairs'].append(dict(source_id=row['id'], copy_id=copied_unit+':'+suffix,
                    identity=proof['identity']+':'+suffix, physical_size=row['physical_size']))
        records.append(proof)
    return records


def attach(records, bindings, candidates=()):
    """Keep separate comparisons, but give proved copies one emitted owner."""
    owners = {}
    for record in records:
        for pair in record['allocation_pairs']:
            for candidate in (pair['source_id'], pair['copy_id']):
                owners[candidate] = pair['identity'], pair['physical_size']
    for row in candidates:
        owner = owners.get(row['id'])
        if owner and owner[1] != row['physical_size']:
            raise ValueError('emission-copy extent disagrees with the raw allocation')
        row['emission_identity'] = owner[0] if owner else ''
    placements = defaultdict(set)
    for binding in bindings:
        if len(binding['candidate_ids']) != 1:
            continue
        owner = owners.get(binding['candidate_ids'][0])
        if owner:
            binding['emission_identity'], binding['emission_physical_size'] = owner
            if binding['rva'] is not None:
                placements[owner[0]].add(binding['rva'])
    for binding in bindings:
        if binding['status'] == 'bound' and len(placements.get(binding.get('emission_identity'), ())) > 1:
            binding['status'] = 'conflicting-emission-placement'
