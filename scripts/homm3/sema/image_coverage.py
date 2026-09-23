"""Actionable, exhaustive accounting in independent file-offset and RVA domains."""
from __future__ import annotations

from collections import Counter, defaultdict
from dataclasses import asdict
import hashlib
import json
from pathlib import Path

from homm3.core import common, tsv
from homm3.sema import retail_claims
from homm3.sema.retail_layout import Layout


def compatible(claims):
    """Allow aliases of one physical extent and explicitly reviewed sharing."""
    if len(claims) < 2:
        return True
    groups = {c.get('shared_group') for c in claims}
    if len(groups) == 1 and None not in groups and '' not in groups:
        return True
    extents = {(c['start'], c['size'], c['kind']) for c in claims}
    if len(extents) == 1:
        return True
    # A validated handler body may have its own admitted function extent.
    functions = [c for c in claims if c['kind'] == 'function']
    nested = [c for c in claims if c['kind'] == 'eh-handler-stub']
    if len(functions) == 1 and len(functions) + len(nested) == len(claims):
        owner = functions[0]
        return all(owner['start'] <= c['start'] and
                   c['start'] + c['size'] <= owner['start'] + owner['size'] for c in nested)
    return False


def reference_facts(root, layout):
    admitted = {int(r['site_rva'], 0) for r in retail_claims.rows(root / 'config/retail/relocs.tsv')}
    evidence = {int(r['site_rva'], 0): r for r in
                retail_claims.rows(root / 'config/retail/reloc-evidence.tsv')}
    references = []
    for site in sorted(admitted | evidence.keys()):
        value = layout.unpack('<I', site)[0]
        prior = evidence.get(site, {})
        references.append(dict(site_rva=site, target_rva=value - layout.base,
                               value=value, admitted=site in admitted,
                               disposition=prior.get('disposition', 'admitted'),
                               channel=prior.get('channel', ''), detail=prior.get('detail', ''),
                               evidence_value=prior.get('value', ''),
                               evidence_value_matches=(int(prior['value'], 0) == value
                                                       if prior.get('value') else None)))
    return references


def validate_claims(layout, claims):
    problems = []
    for i, c in enumerate(claims):
        if c['domain'] not in ('file', 'image') or c['size'] <= 0:
            problems.append(dict(claim=i, reason='invalid claim domain or nonpositive size'))
            continue
        regions = layout.file_regions if c['domain'] == 'file' else layout.image_regions
        covered = sum(max(0, min(c['start'] + c['size'], r.end) - max(c['start'], r.start))
                      for r in regions if r.storage not in ('gap', 'overlay'))
        # File-only claims may legitimately describe an overlay (e.g. certificates).
        if c['domain'] == 'file' and 0 <= c['start'] < c['start'] + c['size'] <= len(layout.data):
            covered = c['size']
        if covered != c['size']:
            problems.append(dict(claim=i, reason='claim extends outside backed section/header storage'))
    return problems


def action(category, storage, incoming, kinds):
    if category == 'conflict':
        return 'Resolve incompatible extent claims using their listed retail evidence.'
    if category == 'provisional':
        return 'Prove the original object boundary and identity; a recognized byte pattern is only a lead.'
    if category == 'unknown':
        if storage == 'zero-fill':
            return 'Recover storage owner and extent from retail accesses and initialization; zeros prove no object boundary.'
        if storage in ('gap', 'overlay', 'raw-tail', 'headers'):
            return 'Inspect bytes and PE placement; admit padding/structure only with independent evidence.'
        if incoming:
            return 'Inspect listed retail references; recover access widths, object boundary and source owner.'
        return 'Inspect bytes and adjacent proven extents; absence of recorded references does not prove padding.'
    if 'function' in kinds:
        return 'Extent admitted (may include embedded tables); inspect source/function matching separately.'
    return 'Extent identified; bind source/candidate storage and verify initializers and pointer referents separately.'


def partition(layout, claims, labels, references):
    """Sweep events, retaining all evidence layers without double-counting bytes."""
    incoming, outgoing = Counter(), Counter()
    for ref in references:
        if ref['admitted']:
            incoming[ref['target_rva']] += 1
            outgoing[ref['site_rva']] += 1
    anchors = set(labels) | set(incoming) | set(outgoing)
    result = []
    for region in layout.file_regions + layout.image_regions:
        events = defaultdict(lambda: [set(), set()])
        for i, c in enumerate(claims):
            if c['domain'] == region.domain:
                start, end = c['start'], c['start'] + c['size']
            elif c['domain'] == 'image' and region.domain == 'file' and region.rva is not None:
                start = region.start + c['start'] - region.rva
                end = start + c['size']
            elif c['domain'] == 'file' and region.domain == 'image' and region.file_offset is not None:
                start = region.start + c['start'] - region.file_offset
                end = start + c['size']
            else:
                continue
            lo, hi = max(start, region.start), min(end, region.end)
            if lo < hi:
                events[lo][1].add(i)
                events[hi][0].add(i)
        points = {region.start, region.end, *events}
        if region.rva is not None:
            points.update(region.start + a - region.rva for a in anchors
                          if region.rva <= a < region.rva + region.size)
        active = set()
        bounds = sorted(points)
        for start, end in zip(bounds, bounds[1:]):
            remove, add = events[start]
            active.difference_update(remove)
            active.update(add)
            ids = sorted(active)
            proven = [claims[i] for i in ids if claims[i]['confidence'] == 'proven']
            provisional = [claims[i] for i in ids if claims[i]['confidence'] != 'proven']
            category = ('conflict' if proven and not compatible(proven) else 'identified' if proven else
                        'provisional' if provisional else 'unknown')
            offset = start - region.start
            rva = region.rva + offset if region.rva is not None else None
            disk = region.file_offset + offset if region.file_offset is not None else None
            point_labels = labels.get(rva, [])
            owners, sources, evidence = set(), set(), set()
            for c in proven:
                owner_labels = labels.get(c['start'], []) if c['domain'] == 'image' else []
                if owner_labels:
                    owners.update(label['name'] for label in owner_labels)
                    sources.update(label['source'] for label in owner_labels if label['source'])
                elif c['owner']:
                    owners.add(c['owner'])
                evidence.add(c['evidence'])
            kinds = sorted({c['kind'] for c in proven})
            evidence.update(c['evidence'] for c in provisional)
            blob = layout.data[disk:disk + min(end - start, 32)] if disk is not None else b''
            result.append(dict(domain=region.domain, start=start, end=end, size=end - start,
                rva=rva, va=layout.base + rva if rva is not None else None, file_offset=disk,
                section=region.section, storage=region.storage, category=category,
                kinds=kinds, owners=sorted(owners), source=sorted(sources),
                labels=sorted({label['name'] for label in point_labels}),
                label_source=sorted({label['source'] for label in point_labels if label['source']}),
                evidence=sorted(evidence), claims=ids,
                shared=len(owners) > 1 or len(proven) > 1,
                sharing=('recorded-symbol-aliases' if len(owners) > 1 and len(proven) == 1 else
                         'compatible-extent-claims' if len(proven) > 1 and category == 'identified' else
                         'conflicting-claims' if len(proven) > 1 else 'none'),
                leads=sorted({c['kind'] + ': ' + c['owner'][:120] for c in provisional}),
                incoming_references=incoming[rva], outgoing_references=outgoing[rva],
                preview_hex=blob.hex(), preview_ascii=''.join(chr(b) if 32 <= b < 127 else '.' for b in blob),
                next_action=action(category, region.storage, incoming[rva], kinds)))
    return result


def audit_partition(rows, domain, total):
    cursor = 0
    for row in (r for r in rows if r['domain'] == domain):
        if row['start'] != cursor or row['end'] <= cursor or row['size'] != row['end'] - cursor:
            raise ValueError(f'{domain}: accounting hole/overlap at 0x{cursor:x}')
        cursor = row['end']
    if cursor != total:
        raise ValueError(f'{domain}: accounting ends at {cursor}, expected {total}')


def generate(root, image, *, jobs=4, build_vendor=False):
    layout = Layout(image.data)
    claims, labels, inputs, limitations = retail_claims.collect(root, layout, image)
    inputs.extend(['config/retail/relocs.tsv', 'config/retail/reloc-evidence.tsv'])
    references = reference_facts(root, layout)
    problems = validate_claims(layout, claims)
    rows = partition(layout, claims, labels, references)
    from homm3.analysis import data_declarations
    from homm3.sema import data_coverage
    declared = data_declarations.extract(root, layout.base, jobs=jobs)
    from homm3.analysis import candidate_data
    candidate_evidence = candidate_data.extract(root, declared)
    rows, data_issues, data_summary = data_coverage.overlay(layout, rows, declared)
    from homm3.analysis import vendor_data
    from homm3.sema import vendor_coverage
    vendor_rows, vendor_issues, vendor_analysis = vendor_data.extract(root, layout, build=build_vendor)
    vendor_rows.extend(vendor_coverage.import_contributions(layout))
    for index, row in enumerate(vendor_rows):
        row['id'] = index
    rows, vendor_summary = vendor_coverage.overlay(layout, rows, vendor_rows)
    vendor_issues.extend(vendor_coverage.overlap_issues(rows))
    vendor_summary['analysis'] = vendor_analysis
    vendor_summary['issue_counts'] = dict(Counter(i['kind'] for i in vendor_issues))
    from homm3.sema import compiler_data
    compiler_rows, compiler_issues = compiler_data.collect(root, layout, claims, labels, references)
    rows, compiler_summary = compiler_data.overlay(rows, compiler_rows)
    compiler_summary['issue_counts'] = dict(Counter(i['kind'] for i in compiler_issues))
    summaries = {}
    for domain, total in [('file', len(image.data)), ('image', layout.image_size)]:
        audit_partition(rows, domain, total)
        domain_rows = [r for r in rows if r['domain'] == domain]
        totals = Counter()
        for row in domain_rows:
            totals[row['category']] += row['size']
        summaries[domain] = dict(total_bytes=total, accounted_bytes=sum(totals.values()),
                                 bytes_by_category=dict(totals), rows=len(domain_rows),
                                 shared_bytes=sum(r['size'] for r in domain_rows if r['shared']))
    implementations = ['scripts/homm3/sema/image_coverage.py', 'scripts/homm3/sema/retail_claims.py',
                       'scripts/homm3/sema/retail_layout.py', 'scripts/homm3/sema/coverage.py',
                       'scripts/homm3/vc6/tryblocks.py', 'scripts/homm3/analysis/data_declarations.py',
                       'scripts/homm3/sema/data_coverage.py', 'build/gen/data-declarations.json',
                       'scripts/homm3/analysis/vendor_data.py', 'scripts/homm3/sema/vendor_coverage.py',
                       'scripts/homm3/build/canonicalize_data_symbols.py', 'scripts/homm3/sema/compiler_data.py']
    inputs.extend(implementations)
    inputs.extend(candidate_evidence['input_sha256'])
    return dict(schema='homm3.retail-accounting.v1', image_base=layout.base,
                image_sha256=hashlib.sha256(image.data).hexdigest(), domains=summaries,
                regions=[asdict(r) for r in layout.file_regions + layout.image_regions],
                data_declarations=declared['declarations'], data_issues=data_issues, data_coverage=data_summary,
                candidate_evidence=candidate_evidence, candidate_coverage=candidate_evidence['summary'],
                vendor_data=vendor_rows, vendor_issues=vendor_issues, vendor_coverage=vendor_summary,
                compiler_data=compiler_rows, compiler_issues=compiler_issues, compiler_coverage=compiler_summary,
                claims=claims, labels=dict(labels), references=references, rows=rows, problems=problems,
                input_sha256={p: hashlib.sha256((root / p).read_bytes()).hexdigest() for p in sorted(set(inputs))},
                limitations=limitations + [
                    'File and image totals are independent domains; do not add them together.',
                    'Labels, candidate sizes and provisional strings are not proof of retail object extents.',
                    'Function extents may include embedded jump/data tables; an extent is not an instruction classification.',
                    'Generated labels/fragments are optional navigation snapshots; regenerate through the normal build after source changes.',
                    'Relocation evidence is incomplete: computed pointer flows and access widths remain unverified.',
                    'Extent identification does not establish source reconstruction, data matching or link closure.'])


def backlog_rows(report):
    """One investigation per address, plus file bytes with no image mapping."""
    return sorted((r for r in report['rows'] if r['category'] != 'identified' and
                   (r['domain'] == 'image' or r['rva'] is None)),
                  key=lambda r: (r['category'] != 'conflict', not r['incoming_references'],
                                 not r['labels'], -r['size'], r['domain'], r['start']))


def export(report, directory):
    directory.mkdir(parents=True, exist_ok=True)
    fields = list(report['rows'][0])
    def render(row):
        out = dict(row)
        for key, value in out.items():
            if isinstance(value, list) or isinstance(value, str) and any(c in value for c in '\t\r\n'):
                out[key] = json.dumps(value, ensure_ascii=True)
            elif value is None:
                out[key] = ''
            elif key in ('start', 'end', 'rva', 'va', 'file_offset', 'site_rva', 'target_rva', 'value'):
                out[key] = hex(value)
        return out
    tsv.write(directory / 'coverage.tsv', ['# Whole retail accounting: file offsets and image RVAs are separate domains; ends exclusive.'],
              fields, [render(r) for r in report['rows']])
    tsv.write(directory / 'backlog.tsv',
              ['# Unresolved image spans plus file-only bytes, prioritized by conflict/reference/label/size.'],
              fields, [render(r) for r in backlog_rows(report)])
    tsv.write(directory / 'claims.tsv', ['# Proven and provisional extent evidence.'],
              ['id', *report['claims'][0]], [render(dict(id=i, **c)) for i, c in enumerate(report['claims'])])
    label_rows = [dict(rva=int(rva), **label) for rva, labels in report['labels'].items() for label in labels]
    tsv.write(directory / 'labels.tsv', ['# Optional navigation snapshots; no extent authority.'],
              ['rva', 'name', 'source', 'evidence'], [render(r) for r in label_rows])
    if report['references']:
        tsv.write(directory / 'references.tsv', ['# Admitted and withheld reference sites; raw words re-read from retail.'],
                  list(report['references'][0]), [render(r) for r in report['references']])
    if 'data_declarations' in report:
        from homm3.sema import data_coverage
        declarations = report['data_declarations']
        tsv.write(directory / 'data-declarations.tsv', ['# Source DATA storage extents; Clang layout is not a VC6 byte verdict.'],
                  list(declarations[0]) if declarations else ['id', 'rva', 'size', 'end', 'source', 'status'],
                  [render(r) for r in declarations])
        tsv.write(directory / 'data-gaps.tsv', ['# No sized DATA declaration covers these image intervals, regardless of retail identification.'],
                  fields, [render(r) for r in data_coverage.gaps(report['rows'])])
        tsv.write(directory / 'data-issues.tsv', ['# Extraction, unknown extent and overlapping declaration diagnostics.'],
                  ['id', 'kind', 'unit', 'source', 'rva', 'end', 'detail', 'declaration_ids'],
                  [render(dict(id=i, **r)) for i, r in enumerate(report['data_issues'])])
    if 'vendor_data' in report:
        from homm3.sema import vendor_coverage
        vendor_rows = report['vendor_data']
        tsv.write(directory / 'vendor-data.tsv', ['# Relocation-anchored vendor contributions and PE imports; rejected/candidate rows do not cover gaps.'],
                  list(vendor_rows[0]) if vendor_rows else ['id', 'library', 'rva', 'size', 'status'],
                  [render(r) for r in vendor_rows])
        tsv.write(directory / 'vendor-issues.tsv', ['# Missing inputs and unmatched admitted code anchors remain explicit.'],
                  ['kind', 'library', 'member', 'rva', 'end', 'vendor_ids', 'detail'], [render(r) for r in report['vendor_issues']])
        tsv.write(directory / 'data-unaccounted.tsv', ['# DATA gaps with no verified vendor contribution or validated compiler/linker structure.'],
                  fields, [render(r) for r in vendor_coverage.unaccounted(report['rows'])])
    if 'compiler_data' in report:
        compiler_rows = report['compiler_data']
        tsv.write(directory / 'compiler-data.tsv', ['# Validated retail structures; ownership gaps remain explicit and candidate bytes are not yet compared.'],
                  list(compiler_rows[0]) if compiler_rows else ['id', 'rva', 'size', 'kind', 'status'],
                  [render(r) for r in compiler_rows])
        tsv.write(directory / 'compiler-issues.tsv', ['# Invalid structures and missing function/class ownership evidence.'],
                  ['compiler_id', 'rva', 'kind', 'detail'], [render(r) for r in report['compiler_issues']])
    if 'candidate_evidence' in report:
        from homm3.analysis import candidate_data
        candidate_data.export(report['candidate_evidence'], directory)
    summary = {k: v for k, v in report.items() if k not in ('claims', 'labels', 'references', 'rows', 'data_declarations', 'data_issues', 'vendor_data', 'vendor_issues', 'compiler_data', 'compiler_issues', 'candidate_evidence')}
    (directory / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')


def run(args):
    image, _ = common.load_image()
    try:
        report = generate(common.HOMM3_DIR, image, jobs=args.jobs, build_vendor=args.build_vendor)
        if args.output:
            export(report, Path(args.output))
    except (ValueError, OSError) as exc:
        from homm3.sema._common import die
        die(str(exc))
    summary = {k: v for k, v in report.items() if k not in ('claims', 'labels', 'references', 'rows', 'data_declarations', 'data_issues', 'vendor_data', 'vendor_issues', 'compiler_data', 'compiler_issues', 'candidate_evidence')}
    if args.json:
        print(json.dumps(summary, indent=2))
    else:
        print('Complete retail accounting (file offsets and image RVAs are separate totals)')
        for domain, totals in report['domains'].items():
            print(f"{domain}: {totals['accounted_bytes']:,}/{totals['total_bytes']:,} bytes accounted; " +
                  ', '.join(f'{k}={v:,}' for k, v in totals['bytes_by_category'].items()))
        print(f"{len(report['problems'])} invalid extents; {len(report['references']):,} reference sites retained.")
        print('DATA coverage: ' + json.dumps(report['data_coverage'], sort_keys=True))
        print('Vendor accounting: ' + json.dumps({k: v for k, v in report['vendor_coverage'].items()
                                                if k != 'analysis'}, sort_keys=True))
        print('Compiler structure accounting: ' + json.dumps(report['compiler_coverage'], sort_keys=True))
        print('Candidate data bindings: ' + json.dumps(report['candidate_coverage'], sort_keys=True))
        if args.output:
            print(f'Actionable byte map: {args.output}/coverage.tsv; prioritized work: {args.output}/backlog.tsv')
        for limitation in report['limitations']:
            print(f'  {limitation}')
    counts = [d['bytes_by_category'] for d in report['domains'].values()]
    data_complete = (report['data_coverage']['analysis_complete'] and
                     not report['data_coverage'].get('uncovered_bytes', 0) and
                     not report['data_coverage'].get('overlap_bytes', 0) and
                     not report['data_coverage'].get('extern-only_bytes', 0) and
                     not report['data_coverage']['issue_counts'])
    return int(bool((args.require_data_complete and not data_complete) or report['problems'] or any(c.get('conflict', 0) for c in counts) or
                    (args.require_complete and any(c.get('unknown', 0) or c.get('provisional', 0) for c in counts))))
