"""Independent, exhaustive retail data accounting; never a candidate match score."""
from __future__ import annotations

from collections import Counter, defaultdict
import csv
import hashlib
import io
import json
from pathlib import Path
import struct

from homm3.core import common, tsv


CATEGORIES = ('object', 'system', 'padding', 'provisional', 'unknown', 'overlap')


def pe_regions(data):
    """Keep raw tails and standalone BSS which the instruction reader omits."""
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[:2] != b'MZ' or data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('expected PE image')
    count = struct.unpack_from('<H', data, pe + 6)[0]
    optional_size = struct.unpack_from('<H', data, pe + 20)[0]
    optional = pe + 24
    if struct.unpack_from('<H', data, optional)[0] != 0x10b:
        raise ValueError('expected PE32')
    sections, regions = [], []
    for i in range(count):
        off = optional + optional_size + i * 40
        name = data[off:off + 8].rstrip(b'\0').decode('latin1')
        virtual, rva, raw, disk = struct.unpack_from('<4I', data, off + 8)
        if raw and disk + raw > len(data):
            raise ValueError(f'{name}: raw extent exceeds file')
        sections.append(dict(name=name, rva=rva, virtual_size=virtual,
                             raw_size=raw, raw_offset=disk))
        if name not in ('.rdata', '.data', '.bss'):
            continue
        bounds = sorted({0, raw, max(raw, virtual), min(raw, virtual) if virtual else raw})
        for lo, hi in zip(bounds, bounds[1:]):
            if lo == hi:
                continue
            storage = ('zero-fill' if lo >= raw else
                       'raw-tail' if virtual and lo >= virtual else 'initialized')
            label = '.bss' if storage == 'zero-fill' or name == '.bss' else name
            regions.append(dict(name=label, section=name, storage=storage,
                                rva=rva + lo, end=rva + hi, size=hi - lo,
                                file_offset=disk + lo if lo < raw else None))
    for left, right in zip(sorted(regions, key=lambda r: r['rva']),
                           sorted(regions, key=lambda r: r['rva'])[1:]):
        if left['end'] > right['rva']:
            raise ValueError('overlapping PE data regions')
    return sections, regions, optional


def system_claims(data, sections, optional):
    """PE import structures, including terminators; no guessed string extents."""
    claims = []
    def add(rva, size, evidence):
        claims.append(dict(rva=rva, size=size, category='system', evidence=evidence))
    def raw(rva, size=1):
        for s in sections:
            if s['rva'] <= rva and rva + size <= s['rva'] + s['raw_size']:
                return s['raw_offset'] + rva - s['rva']
        raise ValueError(f'PE import span 0x{rva:x}+0x{size:x} is not file-backed')
    def string(rva, prefix=0):
        start = raw(rva, prefix + 1)
        s = next(s for s in sections if s['rva'] <= rva < s['rva'] + s['raw_size'])
        end = data.find(b'\0', start + prefix, s['raw_offset'] + s['raw_size'])
        if end < 0:
            raise ValueError('unterminated PE import string')
        return end - start + 1
    directory_count = struct.unpack_from('<I', data, optional + 92)[0]
    if directory_count <= 1:
        return claims
    imports, size = struct.unpack_from('<II', data, optional + 96 + 8)
    if not imports:
        return claims
    raw(imports, size)
    terminated = False
    for offset in range(0, size - 19, 20):
        descriptor = imports + offset
        ilt, timestamp, chain, name, iat = struct.unpack_from('<5I', data, raw(descriptor, 20))
        add(descriptor, 20, 'PE import descriptor')
        if not any((ilt, timestamp, chain, name, iat)):
            terminated = True
            break
        add(name, string(name), 'PE import DLL name including NUL')
        lookup = ilt or iat
        index = 0
        while True:
            thunk = struct.unpack_from('<I', data, raw(lookup + index * 4, 4))[0]
            raw(iat + index * 4, 4)
            add(iat + index * 4, 4, 'PE IAT slot' if thunk else 'PE IAT terminator')
            if lookup != iat:
                add(lookup + index * 4, 4, 'PE import lookup slot' if thunk else 'PE import lookup terminator')
            if not thunk:
                break
            if not thunk & 0x80000000:
                add(thunk, string(thunk, 2), 'PE import hint/name including NUL')
            index += 1
    if not terminated:
        raise ValueError('import directory has no bounded terminator')
    # Several imports may share a hint/name object; count one physical extent.
    return list({(c['rva'], c['size'], c['evidence']): c for c in claims}.values())


def partition(regions, claims, anchors=()):
    """Disjoint end-exclusive spans; overlapping claims never inflate coverage."""
    spans, problems = [], []
    for i, claim in enumerate(claims):
        lo, hi = claim['rva'], claim['rva'] + claim['size']
        covered = sum(max(0, min(hi, r['end']) - max(lo, r['rva'])) for r in regions)
        if claim['size'] <= 0 or covered != claim['size']:
            problems.append(dict(claim=i, reason='extent outside data regions or nonpositive'))
        if claim['category'] not in CATEGORIES[:4]:
            raise ValueError(f"invalid extent category: {claim['category']}")
    for region in regions:
        lo, hi = region['rva'], region['end']
        events = defaultdict(lambda: [set(), set()])
        for i, c in enumerate(claims):
            start, end = max(lo, c['rva']), min(hi, c['rva'] + c['size'])
            if start < end:
                events[start][1].add(i)
                events[end][0].add(i)
        bounds = sorted({lo, hi, *events, *(a for a in anchors if lo < a < hi)})
        active = set()
        for start, end in zip(bounds, bounds[1:]):
            remove, add = events[start]
            active.difference_update(remove)
            active.update(add)
            category = ('overlap' if len(active) > 1 else
                        claims[next(iter(active))]['category'] if active else 'unknown')
            spans.append(dict(region=region['name'], section=region['section'],
                              storage=region['storage'], rva=start, end=end,
                              size=end - start, category=category, claims=sorted(active)))
    return spans, problems


def generate(root, image):
    data = image.data
    sections, regions, optional = pe_regions(data)
    claims = system_claims(data, sections, optional)
    inputs = ['config/retail/vtables.tsv', 'config/retail/relocs.tsv',
              'config/retail/reloc-evidence.tsv', 'config/retail/data-extents.tsv']
    # Older vtable rows omit the optional trailing class column.
    rows = list(csv.DictReader(io.StringIO('\n'.join(
        line for line in (root / inputs[0]).read_text().splitlines()
        if line.strip() and not line.startswith('#'))), delimiter='\t'))
    claims += [dict(rva=int(r['rva'], 0), size=int(r['function_count']) * 4,
                    category='object', evidence='config/retail/vtables.tsv') for r in rows]
    for row in tsv.read(root / inputs[3])[2]:
        if not row['evidence'].strip():
            raise ValueError('reviewed data extent requires evidence')
        claims.append(dict(rva=int(row['rva'], 0), size=int(row['size'], 0),
                           category=row['category'], evidence=row['evidence']))
    symbols_path = root / 'build/gen/symbol_names.csv'
    symbols = []
    if symbols_path.exists():
        symbols = list(csv.DictReader(io.StringIO('\n'.join(
            line for line in symbols_path.read_text().splitlines() if not line.startswith('#')))))
    def inside(rva):
        return any(r['rva'] <= rva < r['end'] for r in regions)
    labels = [dict(rva=int(s['rva'], 0), name=s['name'], provenance=s['provenance'],
                   declared_size=s['size']) for s in symbols if s['kind'] == 'data'
              and inside(int(s['rva'], 0))]
    admitted = {int(r['site_rva'], 0) for r in tsv.read(root / inputs[1])[2]}
    evidence = {int(r['site_rva'], 0): r for r in tsv.read(root / inputs[2])[2]}
    references = []
    for site in sorted(admitted | evidence.keys()):
        section = next((s for s in sections if s['rva'] <= site and
                        site + 4 <= s['rva'] + s['raw_size']), None)
        if section is None:
            raise ValueError(f'relocation site 0x{site:x} is not file-backed')
        value = struct.unpack_from('<I', data, section['raw_offset'] + site - section['rva'])[0]
        target = value - image.image_base
        if inside(site) or inside(target):
            references.append(dict(site_rva=site, target_rva=target, value=value,
                                   admitted=site in admitted, evidence=evidence.get(site),
                                   source_in_data=inside(site), target_in_data=inside(target)))
    # Scan every byte offset, not only aligned slots. Values are leads, never proof
    # of pointer type/ownership. Unadmitted and held-back references remain visible.
    pointer_candidates = []
    for section in sections:
        if section['name'] not in ('.rdata', '.data', '.bss'):
            continue
        for delta in range(max(0, section['raw_size'] - 3)):
            site = section['rva'] + delta
            value = struct.unpack_from('<I', data, section['raw_offset'] + delta)[0]
            if image.image_base <= value < image.image_end and site not in admitted:
                pointer_candidates.append(dict(site_rva=site, target_rva=value - image.image_base,
                                               aligned=site % 4 == 0))
    anchors = {s['rva'] for s in labels} | {r['target_rva'] for r in references if r['target_in_data']}
    spans, problems = partition(regions, claims, anchors)
    incoming = Counter(r['target_rva'] for r in references if r['admitted'] and r['target_in_data'])
    names = defaultdict(list)
    for label in labels:
        names[label['rva']].append(label['name'])
    for span in spans:
        span['incoming_references'] = incoming[span['rva']]
        span['labels'] = names[span['rva']]
    totals = {category: sum(s['size'] for s in spans if s['category'] == category) for category in CATEGORIES}
    by_region = []
    for region in regions:
        counts = Counter()
        for span in spans:
            if region['rva'] <= span['rva'] < region['end']:
                counts[span['category']] += span['size']
        by_region.append(dict(region, bytes_by_category=dict(counts)))
    report = dict(schema='homm3.data-coverage.v1', image_sha256=hashlib.sha256(data).hexdigest(),
                  image_base=image.image_base, regions=by_region, claims=claims, spans=spans,
                  labels=labels, references=references, pointer_candidates=pointer_candidates,
                  problems=problems, bytes_by_category=totals, total_bytes=sum(totals.values()),
                  input_sha256={p: hashlib.sha256((root / p).read_bytes()).hexdigest() for p in inputs},
                  labels_sha256=hashlib.sha256(symbols_path.read_bytes()).hexdigest() if symbols_path.exists() else None,
                  limitations=['Labels are optional generated navigation, never extent evidence.',
                               'Address anchors split unknown spans; they do not establish object boundaries.',
                               'Reference inventory is incomplete; access widths and computed pointer flows are not verified.',
                               'Pointer candidates are numeric coincidences until reviewed.',
                               'Zero bytes and PE raw tails are not automatically padding.',
                               'This report does not establish candidate data matching or linking.'])
    report['unknown_spans_with_incoming_references'] = sum(
        1 for s in spans if s['category'] == 'unknown' and s['incoming_references'])
    report['references_into_unknown'] = sum(
        s['incoming_references'] for s in spans if s['category'] == 'unknown')
    # Explicitly separate comparison enrollment from independent retail coverage.
    report['comparison'] = dict(enrolled_bytes=None, matched_bytes=None, verified_pointer_fields=None,
                                status='not measured by retail coverage')
    return report


def run(args):
    image, _ = common.load_image()
    try:
        report = generate(common.HOMM3_DIR, image)
    except (ValueError, OSError, struct.error) as exc:
        from homm3.sema._common import die
        die(str(exc))
    if args.output:
        directory = Path(args.output)
        directory.mkdir(parents=True, exist_ok=True)
        (directory / 'coverage.json').write_text(json.dumps(report, indent=2) + '\n')
        tsv.write(directory / 'coverage.tsv', ['# Generated retail data coverage; end-exclusive RVAs.'],
                  ['region', 'section', 'storage', 'rva', 'end', 'size', 'category', 'claims', 'incoming_references', 'labels'],
                  [dict(s, rva=hex(s['rva']), end=hex(s['end']), claims=','.join(map(str, s['claims'])),
                        labels=json.dumps(s['labels']))
                   for s in report['spans']])
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print('Retail data coverage (independent of candidate match scores)')
        print('region   storage       RVA range                 bytes    object    system   unknown')
        for r in report['regions']:
            c = r['bytes_by_category']
            print(f"{r['name']:8} {r['storage']:12} {r['rva']:08x}:{r['end']:08x} "
                  f"{r['size']:9,d} {c.get('object', 0):9,d} {c.get('system', 0):9,d} {c.get('unknown', 0):9,d}")
        print('Totals: ' + ', '.join(f'{k}={v:,}' for k, v in report['bytes_by_category'].items()))
        print(f"All {report['total_bytes']:,} bytes accounted for; unknown is not identified.")
        print(f"{len(report['labels']):,} address labels; {len(report['pointer_candidates']):,} unadmitted pointer-value leads; "
              f"{len(report['problems'])} invalid extents.")
        print('Data comparison / candidate pointer verification: not measured.')
        if args.output:
            print(f'Full byte map and reference evidence: {directory}/coverage.{{json,tsv}}')
    return int(bool(report['problems'] or report['bytes_by_category']['overlap'] or
                    (args.require_complete and (report['bytes_by_category']['unknown'] or
                                               report['bytes_by_category']['provisional']))))
