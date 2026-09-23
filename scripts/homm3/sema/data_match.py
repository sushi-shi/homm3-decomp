"""Strict static-data comparison with independent relocation identities.

Read raw fresh COFF and pinned retail directly: no delinker-generated padding,
pointer masks, or function-score history participates in the verdict.
"""
from __future__ import annotations

from collections import Counter, defaultdict
from bisect import bisect_left, bisect_right
import hashlib
import json
from pathlib import Path
import argparse

from homm3.analysis.vendor_data import image_bytes
from homm3.build.canonicalize_data_symbols import CoffObject, RELOCATION_WIDTHS
from homm3.core import tsv

SCHEMA = 'homm3.data-match.v1'
DATA_SECTIONS = {'.rdata', '.data', '.bss'}
PRIORITY = {name: i for i, name in enumerate((
    'unenrolled', 'fixed-match', 'zero-fill-match', 'pointer-match',
    'pointer-unresolved', 'unsupported-relocation', 'missing-relocation',
    'fixed-mismatch', 'zero-fill-mismatch', 'pointer-mismatch', 'binding-conflict'))}


def foldable_literal(coff, row):
    section = coff.sections[row['section_ordinal']-1]
    headers = [s for s in coff.symbols.values() if s.section == section.index and
               s.name == section.name and s.storage_class == 3 and s.aux_count == 1]
    return (section.characteristics & 0x1000 and row['section_offset'] == 0 and
            row['physical_size'] == section.raw_size and len(headers) == 1 and
            coff.data[headers[0].offset+18+14] in (2, 3, 4))


class Identities:
    """Anchors never derive from the pointer field being checked."""
    def __init__(self, candidates, bindings, objects, code_claims=()):
        self.locations = defaultdict(list)
        self.externals = defaultdict(list)
        self.objects = objects
        by_id = {r['id']: r for r in candidates}
        self.candidates = by_id
        self.by_symbol = {(r['unit'], index): r for r in candidates for index in r['symbol_indices']}
        for binding in bindings:
            if binding['status'] != 'bound':
                continue
            row = by_id[binding['candidate_ids'][0]]
            anchor = dict(rva=binding['rva'], size=binding['size'],
                          evidence=binding.get('proof') or 'source-'+binding['macro']+' binding',
                          binding_id=binding['id'], candidate_id=row['id'])
            self.locations[row['unit'], row['section_ordinal']].append((row['section_offset'], anchor))
            for name, scope in zip(row['symbols'], row['scopes']):
                if scope == 'external':
                    self.externals[name].append(anchor)
        for claim in code_claims:
            # Function identities prove entries, not offset correspondence inside
            # non-exact functions. Nonzero code addends remain unresolved.
            anchor = dict(rva=claim['rva'], size=0, evidence=claim['evidence'],
                          binding_id=None, candidate_id='')
            self.externals[claim['symbol']].append(anchor)
            coff = objects.get(claim.get('unit'))
            if coff:
                for symbol in coff.symbols.values():
                    if symbol.section > 0 and symbol.name == claim['symbol'] and symbol.typ & 0x20:
                        self.locations[claim['unit'], symbol.section].append((symbol.value, anchor))

    def resolve(self, unit, symbol, addend):
        anchors = []
        if symbol.section > 0:
            position = symbol.value + addend
            section = self.objects[unit].sections[symbol.section-1]
            section_symbol = bool(symbol.aux_count and symbol.name == section.name and symbol.typ == 0)
            for start, anchor in self.locations[unit, symbol.section]:
                owns_symbol = (section_symbol or start <= symbol.value < start+anchor['size'] or
                               anchor['size'] == 0 and start == symbol.value)
                if owns_symbol and start <= position <= start+anchor['size']:
                    anchors.append(dict(anchor, target_rva=anchor['rva']+position-start,
                                        owner_addend=position-start))
            row = self.by_symbol.get((unit, symbol.index))
            # VC6 narrow-string COMDAT copies have one external linker identity.
            # Reuse a source-bound copy only after checking the complete emitted
            # bytes, not the symbol's hash spelling alone. This uses another
            # declaration as evidence; the pointer under test supplies no anchor.
            if (row and symbol.storage_class == 2 and symbol.name.startswith('??_C@_0') and
                    foldable_literal(self.objects[unit], row) and not row['relocations']):
                for anchor in self.externals[symbol.name]:
                    other = self.candidates.get(anchor['candidate_id'])
                    if (other and foldable_literal(self.objects[other['unit']], other) and not other['relocations'] and
                            other['physical_size'] == row['physical_size'] and
                            other['bytes_sha256'] == row['bytes_sha256'] and 0 <= addend <= anchor['size']):
                        anchors.append(dict(anchor, target_rva=anchor['rva']+addend, owner_addend=addend))
        elif symbol.section == 0:
            for anchor in self.externals[symbol.name]:
                if 0 <= addend <= anchor['size']:
                    anchors.append(dict(anchor, target_rva=anchor['rva']+addend, owner_addend=addend))
        return list({(a['target_rva'], a['candidate_id'], a['owner_addend'], a['evidence']): a
                     for a in anchors}.values())


def enroll(candidates, bindings):
    """Repeated source sites share a projection; overlapping identities conflict."""
    by_id = {r['id']: r for r in candidates}
    projections, grouped = [], {}
    for binding in bindings:
        if binding['status'] != 'bound':
            continue
        key = (binding['candidate_ids'][0], binding['rva'], binding['size'])
        if key in grouped:
            grouped[key]['binding_ids'].append(binding['id'])
            continue
        row = by_id[key[0]]
        projection = dict(id=len(projections), candidate_id=row['id'], unit=row['unit'],
                          rva=binding['rva'], size=binding['size'], binding_ids=[binding['id']],
                          section_ordinal=row['section_ordinal'], section_offset=row['section_offset'],
                          physical_size=row['physical_size'], storage=row['storage'],
                          extent_evidence=binding.get('retail_extent') or 'source projection; retail object boundary unproved',
                          identity=binding['literal_sha256'] or binding.get('source_identity') or binding['name'],
                          linker_identity=binding.get('linker_identity', ''),
                          macro=binding['macro'], status='enrolled')
        grouped[key] = projection
        projections.append(projection)
    ordered = sorted(projections, key=lambda p: p['rva'])
    for i, left in enumerate(ordered):
        for right in ordered[i+1:]:
            if right['rva'] >= left['rva']+left['size']:
                break
            compatible = ((left['rva'], left['size'], left['macro'], left['identity']) ==
                          (right['rva'], right['size'], right['macro'], right['identity']))
            compatible |= bool(left['linker_identity'] and
                (left['rva'], left['size'], left['linker_identity']) ==
                (right['rva'], right['size'], right['linker_identity']))
            if not compatible:
                left['status'] = right['status'] = 'binding-conflict'
    return projections


def runs(statuses):
    start = 0
    for offset in range(1, len(statuses)+1):
        if offset == len(statuses) or statuses[offset] != statuses[start]:
            yield start, offset, statuses[start]
            start = offset


def compare(layout, candidates, bindings, objects, *, code_claims=(), retail_pointers=()):
    projections = enroll(candidates, bindings)
    by_id = {r['id']: r for r in candidates}
    conflicted = {i for p in projections if p['status'] == 'binding-conflict' for i in p['binding_ids']}
    identities = Identities(candidates, [b for b in bindings if b['id'] not in conflicted], objects, code_claims)
    pointer_sites = set(retail_pointers)
    ordered_pointers = sorted(pointer_sites)
    matches, relocs, segments = [], [], []
    for projection in projections:
        row = by_id[projection['candidate_id']]
        size, rva, unit = projection['size'], projection['rva'], projection['unit']
        nearby_pointers = ordered_pointers[bisect_left(ordered_pointers, rva-3):
                                          bisect_left(ordered_pointers, rva+size)]
        coff = objects[unit]
        raw = (bytes(size) if row['allocation'] == 'common-request' else
               coff.section_bytes(coff.sections[row['section_ordinal']-1])[
                   row['section_offset']:row['section_offset']+size])
        if len(raw) != size:
            raise ValueError(f'{row["id"]}: enrollment exceeds emitted bytes')
        retail = image_bytes(layout, rva, size)
        section = next(s for s in layout.sections if s.rva <= rva < s.rva+s.mapped_size)
        statuses = []
        for offset, (base, target) in enumerate(zip(raw, retail)):
            zero = row['storage'] == 'bss' or rva+offset >= section.rva+section.raw_size
            statuses.append(('zero-fill-' if zero else 'fixed-') + ('match' if base == target else 'mismatch'))
        candidate_relocs = []
        if row['section_ordinal']:
            sec = coff.sections[row['section_ordinal']-1]
            sec_raw = coff.section_bytes(sec)
            for ref in coff.relocations:
                width = RELOCATION_WIDTHS.get(ref.typ)
                offset = ref.site-row['section_offset']
                if ref.section != sec.index or offset >= size or offset+(width or 4) <= 0:
                    continue
                candidate_relocs.append(dict(offset=offset, width=width, type=ref.typ,
                    target_index=ref.symbol_index, addend=int.from_bytes(sec_raw[ref.site:ref.site+width], 'little')
                    if width and ref.site+width <= len(sec_raw) else None))
        occupancy = Counter(i for ref in candidate_relocs for i in range(
            max(0, ref['offset']), min(size, ref['offset']+(ref['width'] or 4))))
        valid_sites = set()
        for reloc in candidate_relocs:
            offset, width = reloc['offset'], reloc['width']
            if offset >= size:
                continue
            extent = range(max(0, offset), min(size, offset+(width or 4)))
            malformed = (not width or offset < 0 or offset+width > size or
                         any(occupancy[i] > 1 for i in extent) or any(
                             rva+offset != site and max(rva+offset, site) < min(rva+offset+(width or 4), site+4)
                             for site in nearby_pointers))
            if not malformed and reloc['type'] == 6:
                valid_sites.add(rva+offset)
            symbol = coff.symbols[reloc['target_index']]
            addend = reloc['addend']
            # COFF encodes signed addends in a 32-bit word.
            if addend is not None and addend & 0x80000000:
                addend -= 0x100000000
            anchors = identities.resolve(unit, symbol, addend) if addend is not None else []
            possible = sorted({a['target_rva'] for a in anchors})
            value = int.from_bytes(retail[offset:offset+4], 'little') if 0 <= offset and offset+4 <= size else None
            expected = layout.base + possible[0] if len(possible) == 1 else None
            if malformed or reloc['type'] != 6:
                verdict = 'unsupported-relocation'
            elif expected is None:
                verdict = 'pointer-unresolved'
            else:
                verdict = 'pointer-match' if expected == value else 'pointer-mismatch'
            for i in extent:
                statuses[i] = verdict
            relocs.append(dict(id=len(relocs), projection_id=projection['id'], unit=unit,
                site_rva=rva+offset, offset=offset, type=reloc['type'], width=width,
                target=symbol.name, addend=addend, expected_value=expected, retail_value=value,
                target_rvas=possible, anchors=anchors, status=verdict,
                retail_site_admitted=rva+offset in pointer_sites))
        for site in nearby_pointers:
            offsets = set(range(max(0, site-rva), min(size, site-rva+4)))
            if site not in valid_sites:
                for offset in offsets:
                    statuses[offset] = max(statuses[offset], 'missing-relocation', key=PRIORITY.get)
                relocs.append(dict(id=len(relocs), projection_id=projection['id'], unit=unit,
                    site_rva=site, offset=site-rva, type=None, width=4, target='', addend=None,
                    expected_value=None, retail_value=int.from_bytes(image_bytes(layout, site, 4), 'little'),
                    target_rvas=[], anchors=[], status='missing-relocation', retail_site_admitted=True))
        if projection['status'] == 'binding-conflict':
            statuses = ['binding-conflict'] * size
        counts = Counter(statuses)
        matches.append(dict(projection, status='static-exact' if all(s in (
            'fixed-match', 'zero-fill-match', 'pointer-match') for s in statuses) else 'not-exact',
            bytes_by_status=dict(counts), candidate_sha256=hashlib.sha256(raw).hexdigest(),
            retail_sha256=hashlib.sha256(retail).hexdigest(), initialization='not-verified'))
        segments.extend(dict(start=rva+lo, end=rva+hi, status=status, projection_id=projection['id'])
                        for lo, hi, status in runs(statuses))
    byte_rows = partition(layout, segments)
    counts = Counter()
    for row in byte_rows:
        counts[row['status']] += row['size']
    total = sum(s.mapped_size for s in layout.sections if s.name in DATA_SECTIONS)
    assert sum(counts.values()) == total
    return dict(schema=SCHEMA, enrollment=projections, matches=matches, relocations=relocs,
                byte_verdicts=byte_rows, summary=dict(total_bytes=total, bytes_by_status=dict(counts),
                    enrolled_bytes=total-counts['unenrolled'],
                    matched_initialized_bytes=counts['fixed-match']+counts['pointer-match'],
                    zero_fill_agreement_bytes=counts['zero-fill-match'],
                    static_exact_allocations=sum(r['status'] == 'static-exact' for r in matches),
                    compared_allocations=len(matches), binding_statuses=dict(Counter(b['status'] for b in bindings))))


def partition(layout, segments):
    """The worst verdict wins when multiple candidate copies cover a retail byte."""
    result = []
    for section in sorted(layout.sections, key=lambda s: s.rva):
        if section.name not in DATA_SECTIONS:
            continue
        events = defaultdict(lambda: ([], []))
        events[section.rva]
        events[section.rva+section.mapped_size]
        for index, segment in enumerate(segments):
            if section.rva <= segment['start'] < segment['end'] <= section.rva+section.mapped_size:
                events[segment['start']][0].append(index)
                events[segment['end']][1].append(index)
        active = set()
        cuts = sorted(events)
        for start, end in zip(cuts, cuts[1:]):
            add, remove = events[start]
            active.difference_update(remove)
            active.update(add)
            status = max((segments[i]['status'] for i in active), key=PRIORITY.get, default='unenrolled')
            result.append(dict(rva=start, end=end, size=end-start, section=section.name, status=status,
                               projection_ids=sorted({segments[i]['projection_id'] for i in active})))
    return result


def load_objects(root, hashes, object_paths=None):
    """Validate all inputs, loading only explicitly selected raw COFF objects.

    Private vendor builds can share basenames with source objects. Provenance
    inputs are not an object-selection map and must never overwrite a candidate.
    """
    paths = object_paths
    if paths is None:
        paths = {}
        for p in hashes:
            if p.endswith('.obj'):
                unit = Path(p).stem
                if unit in paths and paths[unit] != p:
                    raise ValueError(f'{unit}: ambiguous raw-object basename; explicit selection required')
                paths[unit] = p
    missing = set(paths.values())-hashes.keys()
    if missing:
        raise ValueError(f'raw objects missing from provenance: {sorted(missing)}')
    selected = defaultdict(list)
    for unit, path in paths.items():
        selected[path].append(unit)
    objects = {}
    for path, expected in hashes.items():
        payload = (root/path).read_bytes()
        if hashlib.sha256(payload).hexdigest() != expected:
            raise ValueError(f'{path}: candidate evidence changed before data comparison')
        if path in selected:
            obj = CoffObject(payload)
            for unit in selected[path]:
                objects[unit] = obj
    return objects


def prepare(root, *, declared=None, candidate_report=None, jobs=4, build_vendor=False):
    """Shared fresh evidence for static bytes, initialization and consumers."""
    from homm3.analysis import candidate_data, data_declarations
    from homm3.core.project import Project
    from homm3.sema.retail_layout import Layout
    layout = Layout(Project(root).image.data)
    declared = declared if declared is not None else data_declarations.extract(root, layout.base, jobs=jobs)
    candidate_report = candidate_report if candidate_report is not None else candidate_data.extract(root, declared)
    if candidate_report['declaration_fingerprint'] != declared['fingerprint']:
        raise ValueError('candidate bindings and source declarations have different fingerprints')
    object_paths = {Path(p).stem: p for p in candidate_report['input_sha256'] if p.endswith('.obj')}
    objects = load_objects(root, candidate_report['input_sha256'], object_paths)
    admitted = {int(r['rva'], 0) for r in tsv.read(root/'config/retail/functions.tsv')[2]}
    code_claims, issues = [], []
    for unit in declared['units']:
        for claim in unit.get('function_claims', []):
            if claim['rva'] in admitted:
                code_claims.append(claim)
            else:
                issues.append(dict(kind='unadmitted-code-anchor', source=claim['path'], rva=claim['rva']))
    paths = ['config/retail/functions.tsv', 'config/retail/relocs.tsv',
             'config/retail/runtime-map.tsv', 'config/retail/zlib-map.tsv',
             'scripts/homm3/sema/data_match.py', 'scripts/homm3/analysis/vendor_data.py']
    for path in paths[2:4]:
        for row in tsv.read(root/path)[2]:
            rva = int(row['rva'], 0)
            if rva in admitted:
                code_claims.append(dict(symbol=row['name'], rva=rva, unit=row.get('unit'), evidence=path))
    from homm3.analysis import vendor_bindings
    vendor = vendor_bindings.extract(root, layout,
        first_id=max((b['id'] for b in candidate_report['data_bindings']), default=-1)+1,
        build=build_vendor, source_objects=objects)
    by_id = {r['id']: r for r in candidate_report['candidate_data']}
    bindings = []
    for binding in candidate_report['data_bindings']:
        binding = dict(binding)
        if binding['status'] == 'bound':
            row = by_id[binding['candidate_ids'][0]]
            if binding['size'] == row['physical_size']:
                binding['linker_identity'] = vendor_bindings.linker_identity(objects[row['unit']], row)
        bindings.append(binding)
    hashes = dict(candidate_report['input_sha256'], **vendor['input_sha256'])
    object_paths.update(vendor['object_paths'])
    objects = load_objects(root, hashes, object_paths)
    candidate_report = dict(candidate_report,
        candidate_data=candidate_report['candidate_data']+vendor['candidate_data'],
        data_bindings=bindings+vendor['data_bindings'], input_sha256=hashes,
        object_paths=object_paths,
        candidate_issues=candidate_report['candidate_issues']+vendor['issues'])
    return dict(layout=layout, declared=declared, candidate_report=candidate_report, objects=objects,
                code_claims=code_claims+vendor['code_claims'], issues=issues, paths=paths,
                vendor_bindings={k: v for k, v in vendor.items() if k not in ('objects', 'candidate_data')})


def generate(root, *, declared=None, candidate_report=None, jobs=4, evidence=None):
    evidence = evidence or prepare(root, declared=declared, candidate_report=candidate_report, jobs=jobs)
    layout, declared, candidate_report, objects, code_claims, issues, paths = (
        evidence[k] for k in ('layout', 'declared', 'candidate_report', 'objects', 'code_claims', 'issues', 'paths'))
    pointers = [int(r['site_rva'], 0) for r in tsv.read(root/paths[1])[2]]
    report = compare(layout, candidate_report['candidate_data'], candidate_report['data_bindings'],
                     objects, code_claims=code_claims, retail_pointers=pointers)
    report['analysis_issues'] = issues + candidate_report['candidate_issues']
    report['withheld_bindings'] = [b for b in candidate_report['data_bindings'] if b['status'] != 'bound']
    report['source_issues'] = declared['issues']
    report['summary']['unavailable_units'] = len(candidate_report['candidate_issues'])
    report['summary']['unadmitted_code_anchors'] = len(issues)
    report['summary']['source_issue_counts'] = dict(Counter(i['kind'] for i in declared['issues']))
    report['retail_sha256'] = hashlib.sha256(layout.data).hexdigest()
    report['policy'] = dict(enrollment='fresh source and code-anchored vendor projections; conflicting identities cannot match',
                           relocations='independent owner/addend proof; unknown or unsupported is not exact',
                           zero_fill='static zero agreement only; dynamic initialization is not verified')
    report['input_sha256'] = dict(candidate_report['input_sha256'], **{
        p: hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths})
    if 'object_paths' in candidate_report:
        report['object_paths'] = candidate_report['object_paths']
    if 'vendor_bindings' in evidence:
        report['vendor_bindings'] = evidence['vendor_bindings']
        report['summary']['vendor_bindings'] = evidence['vendor_bindings']['summary']
    return report


def overlay(layout, rows, report):
    verdicts = report['byte_verdicts']
    starts = [v['rva'] for v in verdicts]
    result, additional, unaccounted = [], 0, 0
    for row in rows:
        if row['section'] not in DATA_SECTIONS or row['rva'] is None:
            result.append(dict(row, data_match_status='not-data', data_projection_ids=[]))
            continue
        lo, end = row['rva'], row['rva']+row['size']
        index = bisect_right(starts, lo)-1
        while lo < end:
            verdict = verdicts[index]
            if not verdict['rva'] <= lo < verdict['end']:
                raise ValueError('data comparison partition does not cover accounting row')
            hi = min(end, verdict['end'])
            offset = lo-row['rva']
            start = row['start']+offset
            disk = row['file_offset']+offset if row['file_offset'] is not None else None
            copied = dict(row, start=start, end=start+hi-lo, size=hi-lo, rva=lo, va=layout.base+lo,
                          file_offset=disk, data_match_status=verdict['status'],
                          data_projection_ids=verdict['projection_ids'])
            if offset:
                copied.update(labels=[], label_source=[], incoming_references=0, outgoing_references=0,
                              coverage_issues=[])
            blob = layout.data[disk:disk+min(hi-lo, 32)] if disk is not None else b''
            copied.update(preview_hex=blob.hex(), preview_ascii=''.join(chr(b) if 32 <= b < 127 else '.' for b in blob))
            if (copied.get('data_accounting_status') == 'unresolved' and
                    verdict['status'] in ('fixed-match', 'pointer-match')):
                copied['data_accounting_status'] = 'candidate-matched'
                if row['domain'] == 'image':
                    additional += hi-lo
            if row['domain'] == 'image' and copied.get('data_accounting_status') == 'unresolved':
                unaccounted += hi-lo
            result.append(copied)
            lo = hi
            index += 1
    return result, dict(additional_accounted_bytes=additional, remaining_unaccounted_bytes=unaccounted)


def exact(report):
    summary = report['summary']
    return (not report.get('analysis_issues') and summary['compared_allocations'] > 0 and
            summary['static_exact_allocations'] == summary['compared_allocations'] and
            all(status == 'bound' or count == 0 for status, count in summary['binding_statuses'].items()) and
            not any(count for kind, count in summary.get('source_issue_counts', {}).items()
                    if kind != 'compgen-extent-unbound'))


def export(report, directory):
    if 'vendor_bindings' in report:
        from homm3.analysis import vendor_bindings
        vendor_bindings.export(report['vendor_bindings'], directory)
    for name, key, default in [('data-enrollment', 'enrollment', ['id', 'rva', 'size', 'status']),
                              ('data-matches', 'matches', ['id', 'rva', 'size', 'status']),
                              ('data-relocations', 'relocations', ['id', 'site_rva', 'status']),
                              ('data-byte-verdicts', 'byte_verdicts', ['rva', 'size', 'status']),
                              ('data-enrollment-issues', 'withheld_bindings', ['id', 'rva', 'status']),
                              ('data-source-issues', 'source_issues', ['kind', 'source', 'detail'])]:
        rows = report.get(key, [])
        rendered = [{k: '' if v is None else json.dumps(v, ensure_ascii=True) if isinstance(v, (list, dict)) or
                     isinstance(v, str) and any(c in v for c in '\t\r\n')
                     else hex(v) if k in ('rva', 'end', 'site_rva', 'expected_value', 'retail_value') else v
                     for k, v in row.items()} for row in rows]
        tsv.write(directory/f'{name}.tsv', ['# Strict static-data verdicts; zero-fill does not prove dynamic initialization.'],
                  list(dict.fromkeys(k for row in rows for k in row)) if rows else default, rendered)
    (directory/'data-match-summary.json').write_text(json.dumps(
        {k: v for k, v in report.items() if k not in ('enrollment', 'matches', 'relocations', 'byte_verdicts',
                                                   'withheld_bindings', 'source_issues', 'vendor_bindings')}, indent=2)+'\n')


def run(args):
    from homm3.core.common import HOMM3_DIR
    report = generate(HOMM3_DIR, jobs=args.jobs)
    export(report, Path(args.output))
    print(json.dumps(report['summary'], indent=2))
    return int(bool(report['analysis_issues']) or args.require_exact and not exact(report))


def main(argv=None):
    from homm3.core.common import HOMM3_DIR
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', default=HOMM3_DIR/'build/data-match')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--require-exact', action='store_true')
    return run(parser.parse_args(argv))


if __name__ == '__main__':
    raise SystemExit(main())
