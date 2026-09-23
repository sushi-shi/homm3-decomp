"""Objdiff views of independently linked, source-enrolled data projections.

These disposable objects contain only admitted allocations, packed without gaps.
Fixed bytes come from raw VC6 COFF. DIR32 words use independently proved target
identities plus their raw addends; retail supplies only the comparison side.
No relocation reaches objdiff, so its relocation masking cannot hide differences.
Unresolved allocations are withheld explicitly, never replaced with retail bytes.
"""
from __future__ import annotations

from collections import defaultdict
import hashlib
import json
from pathlib import Path
import struct

from homm3.analysis.vendor_data import image_bytes
from homm3.build.canonicalize_data_symbols import CoffObject
from homm3.build import compiled_freshness
from homm3.build.normalized_freshness import ValidationContext, write_stamp, freshness_problems
from homm3.core import tsv


def coff(rows):
    """One packed initialized-data section; each symbol has an exact extent.

    Even zero storage is materialized here: this view checks initial bytes,
    not BSS section-kind equivalence or dynamic initialization.
    """
    payload, symbols, strings = bytearray(), bytearray(), bytearray(4)
    for name, raw in rows:
        if not raw:
            raise ValueError('zero-size objdiff data allocation')
        offset = len(strings)
        strings.extend(name.encode('utf-8') + b'\0')
        symbols.extend(struct.pack('<IIIhHBB', 0, offset, len(payload), 1, 0, 2, 0))
        payload.extend(raw)
    struct.pack_into('<I', strings, 0, len(strings))
    header = struct.pack('<HHIIIHH', 0x14c, 1, 0, 60+len(payload), len(rows), 0, 0)
    section = b'.data\0\0\0' + struct.pack('<IIIIIIHHI', 0, 0, len(payload), 60,
                                            0, 0, 0, 0, 0xc0100040)
    return header + section + payload + symbols + strings


def project(layout, report, objects):
    """Return paired images and a ledger explaining every projection's fate."""
    relocations = defaultdict(list)
    for row in report['relocations']:
        relocations[row['projection_id']].append(row)
    pairs, ledger = defaultdict(lambda: ([], [])), []
    unsafe = {'pointer-unresolved', 'unsupported-relocation', 'missing-relocation', 'binding-conflict'}
    for row in report['matches']:
        reasons = sorted(unsafe.intersection(row['bytes_by_status']))
        record = dict(projection_id=row['id'], unit=row['unit'], rva=hex(row['rva']),
                      size=row['size'], status='withheld' if reasons else 'compared', reasons=reasons)
        ledger.append(record)
        if reasons:
            continue
        obj = objects[row['unit']]
        raw = bytearray(obj.section_bytes(obj.sections[row['section_ordinal']-1])[
            row['section_offset']:row['section_offset']+row['size']]) if row['section_ordinal'] else bytearray(row['size'])
        if len(raw) != row['size']:
            raise ValueError('objdiff data projection exceeds raw allocation')
        for reloc in relocations[row['id']]:
            if (reloc['type'] != 6 or reloc['width'] != 4 or reloc['expected_value'] is None or
                    not 0 <= reloc['offset'] <= len(raw)-4):
                raise ValueError('unproved relocation in objdiff data projection')
            struct.pack_into('<I', raw, reloc['offset'], reloc['expected_value'])
        names = sorted(s.name for s in obj.symbols.values() if
                       s.section == row['section_ordinal'] and s.value == row['section_offset'] and
                       not s.aux_count and s.typ == 0)
        name = f"data_{row['rva']:08x}_{row['id']}_{names[0] if names else row['candidate_id']}"
        base, target = pairs[row['unit']]
        base.append((name, bytes(raw)))
        target.append((name, image_bytes(layout, row['rva'], row['size'])))
        record.update(symbol=name, candidate_sha256=hashlib.sha256(raw).hexdigest(),
                      retail_sha256=hashlib.sha256(target[-1][1]).hexdigest())
    return pairs, ledger


def export(root, report):
    from homm3.core.project import Project
    from homm3.sema.retail_layout import Layout
    image = Project(root).image
    layout = Layout(image.data)
    if hashlib.sha256(layout.data).hexdigest() != report['retail_sha256']:
        raise ValueError('retail changed before objdiff data projection')
    objects, inputs = {}, {'retail': image.path}
    for path, expected in report['input_sha256'].items():
        source = root/path
        raw = source.read_bytes()
        if hashlib.sha256(raw).hexdigest() != expected:
            raise ValueError(f'{path}: stale data comparison evidence')
        inputs[path] = source
        if path.endswith('.obj'):
            objects[Path(path).stem] = CoffObject(raw)
    inputs['data_objdiff.py'] = Path(__file__)
    pairs, ledger = project(layout, report, objects)
    directory = root/'build/objdiff/data'
    directory.mkdir(parents=True, exist_ok=True)
    context = ValidationContext()
    entries = []
    outputs = {}
    for unit, (base, target) in sorted(pairs.items()):
        entry = dict(name=unit)
        for side, rows in (('base', base), ('target', target)):
            relative = f'{side}/{unit}.obj'
            output = directory/relative
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_bytes(coff(rows))
            write_stamp(output, inputs, context=context)
            outputs[relative] = output
            entry[side+'_path'] = relative
        entries.append(entry)
    config = directory/'objdiff.json'
    config.write_text(json.dumps(dict(build_base=False, build_target=False,
        watch_patterns=['**/*.obj'], units=entries), indent=2)+'\n')
    write_stamp(config, dict(inputs, **outputs), context=context)
    tsv.write(directory/'enrollment.tsv', [
        '# Packed source projections, not recovered retail section layout. No padding or gap credit.',
        '# Withheld projections remain non-exact in build/data-match; zero bytes do not prove initialization.'],
        ['projection_id', 'unit', 'rva', 'size', 'status', 'reasons', 'symbol',
         'candidate_sha256', 'retail_sha256'],
        [dict(row, reasons=json.dumps(row['reasons'])) for row in ledger])
    return directory


def generate_report(directory):
    """Reject modified/stale views before invoking the actual objdiff binary."""
    from homm3.build import report
    context = ValidationContext()
    config = directory/'objdiff.json'
    problems = freshness_problems(config, context=context)
    if problems:
        raise ValueError('stale objdiff data configuration: ' + '; '.join(problems))
    records = json.loads(config.with_name(config.name+'.stamp.json').read_text())['inputs']
    validate_compiler_inputs([directory/r['path'] for r in records.values()
                             if r['path'].endswith('.compile.json')], context)
    for unit in json.loads(config.read_text())['units']:
        for side in ('base_path', 'target_path'):
            problems = freshness_problems(directory/unit[side], context=context)
            if problems:
                raise ValueError('stale objdiff data view: ' + '; '.join(problems))
    return report.generate(directory)


def validate_compiler_inputs(stamps, context=None):
    """A saved compiler stamp also depends on today's source/include trees."""
    context = context or ValidationContext()
    trees = {}
    for stamp in stamps:
        record = json.loads(stamp.read_text())
        for path, expected in record['files'].items():
            source = Path(path)
            if not source.is_file() or context.digest(source) != expected:
                raise ValueError(f'{stamp}: compiler input changed: {source}')
        for path, expected in record['trees'].items():
            if path not in trees:
                trees[path] = compiled_freshness.tree_digest(Path(path))
            if trees[path] != expected:
                raise ValueError(f'{stamp}: compiler input tree changed: {path}')
