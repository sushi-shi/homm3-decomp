#!/usr/bin/env python3
"""Generate delinker definitions and an exhaustive retail data/gap TSV.

Names come from DATA annotations and independently bound raw COFF symbols.
Only candidate-backed, unambiguous ranges are assigned an owner; vtables have
separately admitted extents. Packed target sections do not claim retail padding
or candidate section placement. Unknown and conflicting ranges stay visible.
"""
from __future__ import annotations

import csv
import sys

from pathlib import Path

from homm3.core import common
from homm3.core import tsv

DATA_OUT = common.HOMM3_DIR / "build/gen/delink_data_manifest.tsv"
SECTIONS_OUT = common.HOMM3_DIR / "build/gen/delink_data_sections.tsv"
BINDINGS_OUT = common.HOMM3_DIR / "build/gen/delink_data_bindings.tsv"
ALIASES = common.HOMM3_DIR / "config/retail/reloc-aliases.tsv"
VTABLES = common.HOMM3_DIR / "config/retail/vtables.tsv"

# byte-exact vostok 1393e24 headers
DATA_HEADER = ("object\trva\tsize\tstorage\talignment\t"
               "section_ordinal\tsection_offset\tscope")
SECTIONS_HEADER = ("object\tordinal\tname\trva\tsize\talignment\t"
                   "characteristics\tchecksum\tcomdat_selection\t"
                   "associative_ordinal\tstorage")
ALIASES_HEADER = ("function_rva\ttarget_rva\tsite_rva\towner\taddend\t"
                  "occurrences")
BINDINGS_HEADER = ("name\tobject\trva\tsize\tstorage\talignment\t"
                   "section_ordinal\tsection_offset\tscope\tprovenance")


def definitions(report, names, layout, vtables=()):
    """Choose one delinker owner per shared allocation without hiding conflicts."""
    from collections import defaultdict
    from homm3.sema.data_match import enroll
    from homm3.analysis.vendor_data import image_bytes
    candidates = {r['id']: r for r in report['candidate_data']}
    grouped = defaultdict(list)
    issues, rows = [], []
    for projection in enroll(report['candidate_data'], report['data_bindings']):
        if projection['status'] != 'enrolled':
            issues.append(dict(rva=projection['rva'], size=projection['size'],
                               candidate_id=projection['candidate_id'], reason='conflicting bindings'))
            continue
        grouped[projection['rva']].append(projection)
    for rva, copies in sorted(grouped.items()):
        # Prefer the authored game TU to a private vendor compilation of it,
        # and a typed source extent to its physical span including padding.
        copies.sort(key=lambda p: (p['unit'].startswith('vendor_'), p['macro'] != 'DATA',
                                   p['extent_kind'] == 'coff-contribution', p['unit'], p['id']))
        projection = copies[0]
        row = candidates[projection['candidate_id']]
        size = projection['size']
        section = next((s for s in layout.sections if
                        s.rva <= rva < rva+size <= s.rva+s.mapped_size and
                        s.name in ('.rdata', '.data', '.bss')), None)
        if (section is None or not row['symbols'] or len(row['symbols']) != 1 or
                len(row['scopes']) != 1 or not row['section_ordinal']):
            issues.append(dict(rva=rva, size=size, candidate_id=row['id'],
                               reason='missing section, unique symbol, or concrete allocation'))
            continue
        # BSS is a zero allocation. Nonzero retail bytes must remain initialized
        # on the target even if the candidate incorrectly declares zero storage.
        storage = section.name[1:]
        if storage == 'data' and row['storage'] == 'bss' and not any(image_bytes(layout, rva, size)):
            storage = 'bss'
        name = names.get(rva)
        if not name or name.startswith(('__data_', 'data_', '__rdata_')):
            name = row['symbols'][0]
        # Numeric compiler temporaries are TU-private and unstable. Bind both
        # sides by the admitted RVA, never by equal initializer bytes.
        if name.startswith(('$', '__h3data$')):
            name = f'__h3data${rva:08x}'
        rows.append(dict(object=row['unit']+'.c', rva=rva, size=size, storage=storage,
                         alignment=1, section_ordinal='-', section_offset='-', scope=row['scopes'][0],
                         name=name, candidate_id=row['id'],
                         candidate_section=row['section_ordinal'], candidate_offset=row['section_offset'],
                         candidate_storage=row['storage'], proof=projection['extent_evidence']))
    # Preserve reviewed vtables which have no candidate storage yet.
    for rva, size in vtables:
        if any(r['rva'] <= rva < r['rva']+r['size'] or rva <= r['rva'] < rva+size for r in rows):
            continue
        rows.append(dict(object='vtables.c', rva=rva, size=size, storage='rdata', alignment=4,
                         section_ordinal='-', section_offset='-', scope='external', name=names[rva],
                         candidate_id='', candidate_section='', candidate_offset='', candidate_storage='',
                         proof='config/retail/vtables.tsv'))
    # A shorter preferred view must never permit a conflicting neighbour.
    ordered = sorted(rows, key=lambda r: r['rva'])
    bad = set()
    for i, left in enumerate(ordered):
        for j in range(i+1, len(ordered)):
            right = ordered[j]
            if right['rva'] >= left['rva']+left['size']:
                break
            bad.update((i, j))
    by_name, by_candidate = defaultdict(list), defaultdict(list)
    for i, row in enumerate(ordered):
        by_name[row['object'], row['name']].append(i)
        if row['candidate_id']:
            by_candidate[row['candidate_id']].append(i)
    for indices in [*by_name.values(), *by_candidate.values()]:
        if len(indices) > 1:
            bad.update(indices)
    for i in sorted(bad):
        row = ordered[i]
        issues.append(dict(rva=row['rva'], size=row['size'], candidate_id=row['candidate_id'],
                           reason='overlapping range or duplicate owner symbol'))
    return [r for i, r in enumerate(ordered) if i not in bad], issues


def accounting(layout, rows, issues):
    """Partition every byte, including zero-fill tails, against delivered rows."""
    result = []
    for section in layout.sections:
        if section.name not in ('.rdata', '.data', '.bss'):
            continue
        start, end = section.rva, section.rva+section.mapped_size
        cuts = {start, end, min(end, start+section.raw_size)}
        for row in [*rows, *issues]:
            cuts.update(max(start, min(end, p)) for p in (row['rva'], row['rva']+row['size']))
        cuts = sorted(cuts)
        for lo, hi in zip(cuts, cuts[1:]):
            owners = [r for r in rows if r['rva'] <= lo < r['rva']+r['size']]
            problems = [r for r in issues if r['rva'] <= lo < r['rva']+r['size']]
            if len(owners) > 1:
                raise ValueError('overlapping delinker definitions')
            owner = owners[0] if owners else {}
            result.append(dict(rva=hex(lo), size=hi-lo, section=section.name,
                backing='zero-fill' if lo >= start+section.raw_size else 'file',
                status='delinked' if owners else 'conflict' if problems else 'gap',
                object=owner.get('object', ''), name=owner.get('name', ''),
                candidate_id=owner.get('candidate_id', ''),
                reason='; '.join(sorted({r['reason'] for r in problems}))))
    return result


def generate(candidate_report=None, *, evidence=None) -> Path:
    from homm3.sema import data_match
    from homm3.analysis import candidate_data
    if evidence is None:
        evidence = data_match.prepare(common.HOMM3_DIR, candidate_report=candidate_report, build_vendor=True)
    report, layout = evidence['candidate_report'], evidence['layout']
    candidate_data.export(report, common.HOMM3_DIR/'build/gen/data')
    with (common.HOMM3_DIR/'build/gen/symbol_names.csv').open() as stream:
        names = {int(r['rva'], 0): r['name'] for r in csv.DictReader(
            line for line in stream if not line.startswith('#')) if r['kind'] == 'data'}
    vtables = [(int(r[0], 0), int(r[1])*4) for line in VTABLES.read_text().splitlines()
               if line and not line.startswith(('#', 'rva')) for r in [line.split('\t')]]
    rows, issues = definitions(report, names, layout, vtables)
    provenance = common.provenance('homm3.build.data_manifest')
    DATA_OUT.parent.mkdir(parents=True, exist_ok=True)
    serialized = [dict(r, rva=hex(r['rva']), size=hex(r['size']), alignment=hex(r['alignment'])) for r in rows]
    tsv.write(DATA_OUT, ['# GENERATED - consumed by vostok --data-manifest.', *provenance],
              DATA_HEADER.split('\t'), serialized)
    # No guessed section topology: Vostok packs the admitted allocations.
    tsv.write(SECTIONS_OUT, ['# Packed definitions; no admitted section topology.'],
              SECTIONS_HEADER.split('\t'), [])
    from homm3.sema.data_match import enroll
    candidates = {r['id']: r for r in report['candidate_data']}
    selected = {r['rva']: r for r in rows}
    bindings, seen = [], set()
    for projection in enroll(report['candidate_data'], report['data_bindings']):
        owner = selected.get(projection['rva'])
        if not owner or projection['status'] != 'enrolled':
            continue
        candidate = candidates[projection['candidate_id']]
        if not candidate['section_ordinal'] or len(candidate['symbols']) != 1 or len(candidate['scopes']) != 1:
            continue
        key = candidate['id']
        if key in seen:
            continue
        seen.add(key)
        bindings.append(dict(name=owner['name'], object=candidate['unit']+'.c',
            rva=hex(owner['rva']), size=hex(min(owner['size'], projection['size'])),
            alignment=candidate['alignment'], section_ordinal=candidate['section_ordinal'],
            section_offset=hex(candidate['section_offset']), storage=candidate['storage'],
            scope=candidate['scopes'][0], provenance='delink-definition',
            symbol=candidate['symbols'][0], object_sha256=candidate['object_sha256']))
    tsv.write(BINDINGS_OUT, ['# Candidate identities for the same delinker definitions.'],
              [*BINDINGS_HEADER.split('\t'), 'symbol', 'object_sha256'], bindings)
    tsv.write(DATA_OUT.with_name('delink_data_symbols.tsv'), [],
              ['rva', 'name'], serialized)
    tsv.write(DATA_OUT.with_name('delink_data_issues.tsv'), [],
              ['rva', 'size', 'candidate_id', 'reason'], issues)
    ledger = accounting(layout, rows, issues)
    tsv.write(common.HOMM3_DIR/'build/gen/data/retail-data.tsv', [
        '# Every retail .rdata/.data/.bss byte. Delinked means delivered, not matched.',
        '# Zero-fill agreement does not prove dynamic initialization.'],
        ['rva', 'size', 'section', 'backing', 'status', 'object', 'name', 'candidate_id', 'reason'], ledger)
    vendor_units = sorted({r['object'][:-2] for r in rows if r['object'].startswith('vendor_')})
    tsv.write(DATA_OUT.with_name('delink_data_units.tsv'), [], ['unit', 'path'],
              [dict(unit=u, path=evidence['candidate_report']['object_paths'][u]) for u in vendor_units]
              + ([dict(unit='vtables', path='')] if any(r['object'] == 'vtables.c' for r in rows) else []))
    if not ALIASES.is_file():
        ALIASES.write_text('# MANUALLY MANAGED - reviewed relocation aliases.\n'+ALIASES_HEADER+'\n')
    delivered = sum(r['size'] for r in ledger if r['status'] == 'delinked')
    total = sum(r['size'] for r in ledger)
    print(f'[build data_manifest] {len(rows)} definitions; {delivered:,}/{total:,} bytes delivered; '
          f'{total-delivered:,} gap/conflict bytes -> build/gen/data/retail-data.tsv')
    return DATA_OUT


def verify_delivery():
    """Every credited range must survive into a target opened by objdiff."""
    import json
    from homm3.build import canonicalize_data_symbols as canon
    root = common.HOMM3_DIR/'build/objdiff'
    units = {r['name']: r for r in json.loads((root/'objdiff.json').read_text())['units']}
    names, objects = data_names(), {}
    for row in tsv.read(DATA_OUT)[2]:
        unit = row['object'][:-2]
        if unit not in units or units[unit]['target_path'].endswith('dummy.obj'):
            raise ValueError(f'{unit}: delinked data is absent from objdiff')
        if unit not in objects:
            objects[unit] = canon.CoffObject((root/units[unit]['target_path']).read_bytes())
        obj = objects[unit]
        name = canon.normalize_anon_ns_name(names[int(row['rva'], 0)], unit)
        symbols = [s for s in obj.symbols.values() if s.name == name and s.section > 0]
        if len(symbols) != 1:
            raise ValueError(f'{unit}: delinked data {name} has {len(symbols)} target definitions')
        symbol = symbols[0]
        section = obj.sections[symbol.section-1]
        if symbol.value+int(row['size'], 0) > section.raw_size:
            raise ValueError(f'{unit}: delinked data {name} was truncated before objdiff')


def data_names():
    return {int(r['rva'], 0): r['name'] for r in tsv.read(DATA_OUT.with_name('delink_data_symbols.tsv'))[2]}


def extra_units():
    path = DATA_OUT.with_name('delink_data_units.tsv')
    return tsv.read(path)[2] if path.is_file() else []


def main(argv=None) -> int:
    generate()
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
