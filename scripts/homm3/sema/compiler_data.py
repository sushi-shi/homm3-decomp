"""Account validated compiler/linker structures without claiming candidate matches."""
from bisect import bisect_right
from collections import Counter, defaultdict
import struct

from homm3.sema.retail_claims import rows as table_rows

EH_KINDS = {'eh-funcinfo', 'eh-unwind-map', 'eh-try-map', 'eh-catch-map'}


def handler_owners(layout, claims, references):
    """Attribute a handler only through a decoded push in an admitted function."""
    import capstone
    functions = sorted((c['start'], c['size']) for c in claims if c['kind'] == 'function')
    starts = [start for start, _ in functions]
    handlers = {c['start'] for c in claims if c['kind'] == 'eh-handler-stub'}
    owners, decoded = defaultdict(set), {}
    disassembler = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for ref in references:
        if ref['target_rva'] not in handlers or not ref['admitted']:
            continue
        site = ref['site_rva']
        index = bisect_right(starts, site) - 1
        if index < 0:
            continue
        start, size = functions[index]
        if not start <= site < site + 4 <= start + size:
            continue
        if start not in decoded:
            decoded[start] = {address + 1 for address, length, mnemonic, _ in
                              disassembler.disasm_lite(layout.read(start, size), start)
                              if mnemonic == 'push' and length == 5}
        if site in decoded[start] and layout.unpack('<I', site)[0] == layout.base + ref['target_rva']:
            owners[ref['target_rva']].add(start)
    return owners


def collect(root, layout, claims, labels, references):
    """Join existing proven extent claims to structured compiler ownership facts."""
    classes = {int(r['rva'], 0): r.get('class', '') for r in
               table_rows(root / 'config/retail/vtables.tsv')}
    owners = handler_owners(layout, claims, references)
    stubs = defaultdict(set)
    for claim in claims:
        if claim['kind'] == 'eh-handler-stub':
            stubs[layout.unpack('<I', claim['start'] + 1)[0] - layout.base].add(claim['start'])
    # Shared maps may belong to several FuncInfo records. Keep every association.
    associations = defaultdict(set)
    for claim in claims:
        if claim['kind'] != 'eh-funcinfo' or claim['confidence'] != 'proven':
            continue
        info = claim['start']
        _, count, unwind, tries, try_map, _, _ = layout.unpack('<7I', info)
        extents = [('eh-funcinfo', info, 28)]
        if count:
            extents.append(('eh-unwind-map', unwind - layout.base, count * 8))
        if tries:
            start = try_map - layout.base
            extents.append(('eh-try-map', start, tries * 20))
            for index in range(tries):
                catches, handlers = layout.unpack('<II', start + index * 20 + 12)
                if catches:
                    extents.append(('eh-catch-map', handlers - layout.base, catches * 16))
        for extent in extents:
            associations[extent].add(info)

    result, issues = [], []
    for claim_id, claim in enumerate(claims):
        kind, start, size = claim['kind'], claim['start'], claim['size']
        if claim['confidence'] != 'proven' or claim['domain'] != 'image':
            continue
        terminator = (kind == 'import-structure' and size == 20 and
                      claim['evidence'] == 'PE import descriptor' and layout.read(start, size) == bytes(20))
        if kind not in EH_KINDS and kind != 'vtable' and not terminator:
            continue
        info_rvas = sorted(associations.get((kind, start, size), ()))
        handler_rvas = sorted({stub for info in info_rvas for stub in stubs[info]})
        owner_rvas = sorted({owner for stub in handler_rvas for owner in owners[stub]})
        class_name = classes.get(start, '') if kind == 'vtable' else ''
        names = sorted({label['name'] for owner in owner_rvas for label in labels.get(owner, [])})
        sources = sorted({label['source'] for owner in owner_rvas for label in labels.get(owner, []) if label['source']})
        missing, invalid = [], []
        if kind in EH_KINDS:
            if not info_rvas or not handler_rvas:
                invalid.append('EH extent has no validated FuncInfo/handler association')
            if not owner_rvas:
                missing.append('No admitted, instruction-bound handler push establishes a function owner')
        targets = []
        if kind == 'vtable':
            targets = [value - layout.base for value, in struct.iter_unpack('<I', layout.read(start, size))]
            if not targets or any(not any(s.name == '.text' and s.rva <= target < s.rva + s.raw_size
                                          for s in layout.sections) for target in targets):
                invalid.append('Vtable slot does not point into file-backed retail code')
            if not class_name:
                missing.append('Class identity is not admitted in the retail vtable inventory')
        row = dict(id=len(result), claim_id=claim_id, rva=start, end=start + size, size=size,
                   kind='import-terminator' if terminator else kind,
                   status='invalid' if invalid else 'accounted',
                   owner_kind='linker' if terminator else 'class' if class_name else 'function' if owner_rvas else 'unresolved',
                   class_name=class_name, owner_rvas=[hex(rva) for rva in owner_rvas],
                   owner_names=names, owner_sources=sources, info_rvas=[hex(rva) for rva in info_rvas],
                   handler_rvas=[hex(rva) for rva in handler_rvas], slot_targets=[hex(rva) for rva in targets],
                   evidence=[claim['evidence']], ownership_gaps=missing,
                   candidate_status='not-compared', candidate_matched_bytes=0)
        result.append(row)
        for detail in invalid + missing:
            issues.append(dict(compiler_id=row['id'], rva=start, kind='invalid-structure' if detail in invalid else
                               'unresolved-owner', detail=detail))
    return result, issues


def overlay(rows, structures):
    """The original claim partition already contains every structural boundary."""
    by_claim = defaultdict(list)
    for structure in structures:
        by_claim[structure['claim_id']].append(structure)
    result, by_kind = [], Counter()
    counts = Counter({key: 0 for key in ('structural_bytes', 'newly_accounted_gap_bytes',
                                       'remaining_unaccounted_bytes', 'unresolved_owner_bytes',
                                       'candidate_compared_bytes', 'candidate_matched_bytes')})
    for row in rows:
        cs = [c for claim_id in row['claims'] for c in by_claim[claim_id]]
        valid = [c for c in cs if c['status'] == 'accounted']
        state = 'conflict' if cs and row['category'] == 'conflict' else 'accounted' if valid else 'invalid' if cs else 'unattributed'
        copied = dict(row, compiler_status=state, compiler_ids=[c['id'] for c in cs],
                      compiler_kinds=sorted({c['kind'] for c in cs}),
                      compiler_owner_rvas=sorted({r for c in cs for r in c['owner_rvas']}),
                      compiler_owner_names=sorted({n for c in cs for n in c['owner_names']}),
                      compiler_classes=sorted({c['class_name'] for c in cs if c['class_name']}),
                      compiler_ownership_gaps=sorted({m for c in cs for m in c['ownership_gaps']}),
                      compiler_candidate_status='not-compared' if cs else 'no-binding')
        if state == 'accounted' and row['data_accounting_status'] == 'unresolved':
            copied['data_accounting_status'] = 'compiler'
            copied['data_next_action'] = 'Retail compiler/linker structure accounted; bind emitted candidate storage and compare bytes and referents.'
        result.append(copied)
        if row['domain'] != 'image' or row['data_coverage_status'] == 'not-data-scope':
            continue
        size = row['size']
        if state == 'accounted':
            counts['structural_bytes'] += size
            if not any(c['owner_kind'] != 'unresolved' for c in valid):
                counts['unresolved_owner_bytes'] += size
            by_kind['+'.join(sorted({c['kind'] for c in valid}))] += size
            if row['data_accounting_status'] == 'unresolved':
                counts['newly_accounted_gap_bytes'] += size
        if copied['data_accounting_status'] == 'unresolved':
            counts['remaining_unaccounted_bytes'] += size
    return result, dict(counts, by_kind=dict(by_kind),
                        statement='Validated retail structure accounting only; candidate bytes and relationships have not been compared.')
