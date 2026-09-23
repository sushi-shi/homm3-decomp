"""Fresh emitted data and conservative source/retail identity bindings.

A binding is not a byte match or a proved retail extent. COFF allocations may
include padding. COMMON is a linker allocation request, not initialized bytes.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re

from homm3.build import compiled_freshness
from homm3.analysis import data_symbols
from homm3.build.canonicalize_data_symbols import CoffObject, RELOCATION_WIDTHS
from homm3.core import tsv
from homm3.core.project import Project

SCHEMA = 'homm3.candidate-data.v1'


def storage(section):
    flags = section.characteristics
    if (section.name.startswith('.debug') or section.name == '.drectve' or
            flags & (0x20 | 0x20000000 | 0x200 | 0x800)):
        return None
    if flags & 0x80:
        return 'bss'
    if flags & 0x40:
        return 'data' if flags & 0x80000000 else 'rdata'
    return None


def inventory(unit, coff, object_sha256):
    """Partition every linkable data section; keep aliases and unnamed gaps."""
    rows = []
    symbols = list(coff.symbols.values())
    for section in coff.sections:
        kind = storage(section)
        if not kind or not section.raw_size:
            continue
        by_offset = defaultdict(list)
        for symbol in symbols:
            if (symbol.section == section.index and symbol.typ == 0 and
                    symbol.storage_class in (2, 3) and not symbol.aux_count):
                if not 0 <= symbol.value <= section.raw_size:
                    raise ValueError(f'{unit}: symbol {symbol.name} outside data section')
                by_offset[symbol.value].append(symbol)
        cuts = sorted({0, section.raw_size, *by_offset})
        raw = coff.section_bytes(section)
        alignment_bits = (section.characteristics >> 20) & 15
        alignment = 1 << (alignment_bits - 1) if 1 <= alignment_bits <= 14 else None
        for start, end in zip(cuts, cuts[1:]):
            owners = by_offset[start]
            relocs = []
            for reloc in coff.relocations:
                if reloc.section != section.index or not start <= reloc.site < end:
                    continue
                target = coff.symbols[reloc.symbol_index]
                width = RELOCATION_WIDTHS.get(reloc.typ)
                relocs.append(dict(offset=reloc.site-start, type=reloc.typ, width=width,
                                   target=target.name, target_index=target.index,
                                   addend=int.from_bytes(raw[reloc.site:reloc.site+width], 'little')
                                   if width and reloc.site+width <= end else None))
            rows.append(dict(id=f'{unit}:{section.index}:{start}', unit=unit,
                             section=section.name, section_ordinal=section.index,
                             section_offset=start, physical_size=end-start, storage=kind,
                             alignment=alignment, characteristics=section.characteristics,
                             symbols=[s.name for s in owners], symbol_indices=[s.index for s in owners],
                             scopes=['external' if s.storage_class == 2 else 'local' for s in owners],
                             allocation='section-span',
                             bytes_sha256=hashlib.sha256(raw[start:end]).hexdigest(),
                             object_sha256=object_sha256, relocations=relocs))
    for symbol in symbols:
        if symbol.section == 0 and symbol.value and symbol.storage_class == 2:
            rows.append(dict(id=f'{unit}:common:{symbol.index}', unit=unit,
                             section='COMMON', section_ordinal=0, section_offset=0,
                             physical_size=symbol.value, storage='bss', alignment=None,
                             characteristics=0, symbols=[symbol.name], symbol_indices=[symbol.index],
                             scopes=['external'], allocation='common-request', bytes_sha256='',
                             object_sha256=object_sha256, relocations=[]))
    return rows


def byte_literal(expression):
    """A deliberately restricted C narrow-string decoder; reject unknown syntax.

    Accept ASCII, ordinary escapes, octal and hex byte escapes and adjacent
    literals. Wide strings, macros, non-ASCII execution encodings and arithmetic
    need compiler expression evidence, not Python's different literal semantics.
    """
    token = re.compile(r'\s*"((?:[^"\\\n]|\\[^\n])*)"')
    parts, cursor = [], 0
    while cursor < len(expression.rstrip()):
        match = token.match(expression, cursor)
        if not match:
            return None
        parts.append(match.group(1))
        cursor = match.end()
    if not parts:
        return None
    result = bytearray()
    escapes = dict(zip('abfnrtv\\\'"?', [7, 8, 12, 10, 13, 9, 11, 92, 39, 34, 63]))
    for part in parts:
        i = 0
        while i < len(part):
            char = part[i]
            i += 1
            if char != '\\':
                if ord(char) > 127 or ord(char) < 32:
                    return None
                result.append(ord(char))
                continue
            char = part[i]
            i += 1
            if char in escapes:
                value = escapes[char]
            elif char in '01234567':
                digits = char
                while i < len(part) and len(digits) < 3 and part[i] in '01234567':
                    digits += part[i]
                    i += 1
                value = int(digits, 8)
            elif char == 'x':
                start = i
                while i < len(part) and part[i] in '0123456789abcdefABCDEF':
                    i += 1
                if i == start:
                    return None
                value = int(part[start:i], 16)
            else:
                return None
            if value > 255:
                return None
            result.append(value)
    return bytes(result) + b'\0'


def symbol_key(symbol):
    # All admitted bridges preserve this leading name. This is only a search
    # index; spelling() still checks the entire mangled symbol and provenance.
    if symbol.startswith('_?'):
        symbol = symbol[1:]
    return symbol.split('@', 1)[0] if symbol.startswith('?') else symbol


def named_index(rows):
    index = defaultdict(dict)
    for row in rows:
        for symbol in row['symbols']:
            index[symbol_key(symbol)][row['id']] = row
    return index


def named_matches(declaration, rows, *, index=None):
    """Keep all compatible emissions, including alternatives to an exact name."""
    units = set(declaration['definition_units'])
    facts = [*declaration.get('definitions', [])]
    # Summaries retain original observed spellings for exact matching. Bridges
    # use per-TU typed definitions, not properties borrowed across header uses.
    for symbol in declaration['symbols']:
        facts.append(dict(symbol=symbol))
        if 'unit' in declaration:
            facts.append(dict(declaration, symbol=symbol))
        else:
            for unit in declaration['units']:
                facts.append(dict(declaration, symbol=symbol, unit=unit))
    index = named_index(rows) if index is None else index
    keys = {symbol_key(f.get('symbol', '')) for f in facts}
    keys.add('_'+declaration['name'])
    candidates = {key: row for name in sorted(keys) for key, row in index.get(name, {}).items()}
    matches = {}
    for row in candidates.values():
        if units and row['unit'] not in units:
            continue
        proofs = []
        for fact in facts:
            if fact.get('unit') and fact['unit'] != row['unit']:
                continue
            for symbol in row['symbols']:
                proof = data_symbols.spelling(fact, symbol, row['unit'])
                if proof and proof not in proofs:
                    proofs.append(proof)
        if proofs:
            matches[row['id']] = proofs
    return matches


def named_candidates(declaration, rows):
    matches = named_matches(declaration, rows)
    return [row for row in rows if row['id'] in matches]


def guard_candidates(site, function, rows, coff):
    """Join local owner and byte guard in one compiler scope and function graph.

    VC6's @4EA type is an unsigned-char local static; the physical span may
    include padding. Never assume every guard is a dword or search for zeros.
    """
    owner_name = site['expression'].strip()
    if not function or not re.fullmatch(r'[A-Za-z_]\w*', owner_name):
        return []
    owners = [s for s in coff.symbols.values() if s.section > 0 and
              s.name.startswith('_?' + owner_name + '@') and '?' + function + '@4' in s.name]
    funcs = [s for s in coff.symbols.values() if s.section > 0 and s.typ & 0x20]
    if len(owners) != 1:
        return []
    owner = owners[0]
    scope_end = owner.name.index('?' + function + '@4') + len('?' + function)
    scope = owner.name[len('_?' + owner_name + '@'):scope_end]
    guard_symbols = [s for s in coff.symbols.values() if
                     re.fullmatch(r'_\?\$S\d+@' + re.escape(scope) + '@4EA', s.name)]
    indices = set()
    for func in funcs:
        end = min([s.value for s in funcs if s.section == func.section and s.value > func.value]
                  or [coff.sections[func.section-1].raw_size])
        targets = {r.symbol_index for r in coff.relocations if r.section == func.section and
                   func.value <= r.site < end}
        if owner.index in targets:
            # The original helper may be inlined. Its owner and guard retain
            # that helper's mangled scope inside the emitted caller's graph.
            indices.update(s.index for s in guard_symbols if s.index in targets)
    return [r for r in rows if indices.intersection(r['symbol_indices']) and r['physical_size'] >= 1]


def bind(declared, rows, objects):
    bindings = []
    names = named_index(rows)
    source_names = defaultdict(list)
    for unit in declared['units']:
        if unit.get('errors'):
            continue
        for fact in unit.get('definitions', []):
            keys = {symbol_key(fact['symbol'])}
            if fact.get('linkage') == 'INTERNAL' and not fact.get('local', True):
                keys.add('_'+fact['name'])
            for key in keys:
                source_names[unit['unit'], key].append(fact)
    by_unit = defaultdict(list)
    for row in rows:
        by_unit[row['unit']].append(row)
    addresses = defaultdict(set)
    for declaration in declared['declarations']:
        addresses[declaration['usr']].add(declaration['rva'])

    def add(name, macro, source, rva, size, choices, status, proof, declaration_id='', units=None,
            literal_sha256=''):
        bindings.append(dict(id=len(bindings), name=name, macro=macro, source=source,
                             declaration_id=declaration_id, rva=rva, size=size,
                             units=units or sorted({r['unit'] for r in choices}),
                             candidate_ids=sorted({r['id'] for r in choices}), status=status,
                             proof=proof, retail_extent='unproved', candidate_match='not-compared'))
        bindings[-1]['literal_sha256'] = literal_sha256
        bindings[-1]['source_identity'] = ''
        bindings[-1]['symbol_matches'] = {}

    for declaration in declared['declarations']:
        matches = named_matches(declaration, rows, index=names)
        choices = [row for row in rows if row['id'] in matches]
        size = declaration['size']
        status = declaration['status'] if declaration['status'] != 'sized' else 'bound'
        if status == 'bound':
            if len(addresses[declaration['usr']]) > 1:
                status = 'conflicting-retail-address'
            elif not choices:
                status = 'missing-emission'
            elif len(choices) != 1:
                status = 'multiple-emitted-owners'
            elif any(
                    f.get('usr') != declaration['usr'] and
                    data_symbols.spelling(f, symbol, choices[0]['unit'])
                    for symbol in choices[0]['symbols']
                    for f in source_names[choices[0]['unit'], symbol_key(symbol)]):
                status = 'ambiguous-source-definition'
            elif size > choices[0]['physical_size']:
                status = 'truncated-storage'
            elif choices[0]['allocation'] == 'common-request' and size != choices[0]['physical_size']:
                status = 'common-size-conflict'
        add(declaration['name'], 'DATA', declaration['source'], declaration['rva'], size,
            choices, status, 'compiler declaration and checked emitted ABI spelling',
            declaration['id'], declaration['units'])
        bindings[-1]['source_identity'] = declaration['usr']
        bindings[-1]['symbol_matches'] = matches

    active = defaultdict(dict)
    for unit in declared['units']:
        if unit['errors']:
            continue
        for site in unit.get('active_macros', []):
            active[(site['path'], site['offset'], site['macro'])][unit['unit']] = site.get('function_symbol', '')
    for site in declared['sites']:
        if site['macro'] == 'DATA':
            continue
        units = active[(site['path'], site['offset'], site['macro'])]
        payload = byte_literal(site['expression']) if site['macro'] == 'DATA_COMPGEN' else None
        size = len(payload) if payload is not None else None
        for unit in sorted(units) or ['']:
            choices = []
            if not unit:
                status = 'inactive-or-unparsed-site'
            elif unit not in objects:
                status = 'missing-fresh-object'
            elif site['macro'].endswith('_GUARD'):
                choices = guard_candidates(site, units[unit], by_unit[unit], objects[unit])
                status = 'bound' if len(choices) == 1 else 'guard-needs-owner-proof'
                size = 1 if status == 'bound' else None
            elif payload is None:
                status = 'unsupported-expression'
            else:
                coff = objects[unit]
                for row in by_unit[unit]:
                    # Never search arbitrary zero/constant bytes. VC6 narrow
                    # string COMDATs can be writable, and the empty string can
                    # be zero-fill. The compiler symbol proves the pool family.
                    comdat_string = any(s.startswith('??_C@_0') for s in row['symbols'])
                    if (row['relocations'] or not (comdat_string or
                            any(re.fullmatch(r'\$SG\d+', s) for s in row['symbols']))):
                        continue
                    sec = coff.sections[row['section_ordinal']-1]
                    start = row['section_offset']
                    allocation = coff.section_bytes(sec)[start:start+row['physical_size']]
                    if (size <= len(allocation) and allocation[:size] == payload and
                            (size == len(allocation) if comdat_string else
                             not any(allocation[size:]) and len(allocation)-size < (row['alignment'] or 1))):
                        choices.append(row)
                status = 'bound' if len(choices) == 1 else 'ambiguous-pool' if choices else 'missing-pool-emission'
            add(site['name'], site['macro'], f"{site['path']}:{site['line']}", site['rva'], size,
                choices, status, 'active macro; typed guard and local owner referenced by same function' if
                site['macro'].endswith('_GUARD') else 'active source macro; unique emitted narrow-string pool allocation',
                units=[unit] if unit else [], literal_sha256=hashlib.sha256(payload).hexdigest()
                if payload is not None else '')

    # One candidate allocation cannot silently represent distinct retail starts.
    # Identical strings at different addresses are an identity ambiguity too.
    by_candidate = defaultdict(list)
    for binding in bindings:
        if binding['status'] == 'bound':
            by_candidate[binding['candidate_ids'][0]].append(binding)
    for uses in by_candidate.values():
        if len({b['rva'] for b in uses}) > 1:
            for binding in uses:
                binding['status'] = 'conflicting-retail-address'
    by_literal_rva = defaultdict(list)
    for binding in bindings:
        if binding['literal_sha256']:
            by_literal_rva[binding['rva']].append(binding)
    for uses in by_literal_rva.values():
        if len({b['literal_sha256'] for b in uses}) > 1:
            for binding in uses:
                binding['status'] = 'conflicting-literal-value'
    return bindings


def extract(root, declared):
    project = Project(root)
    rows, issues, objects = [], [], {}
    toolchain = project.toolchain
    includes = [toolchain/'include', *(p for p in project.includes if p.is_dir())]
    # Snapshot trees once per source directory within this read-only extraction.
    snapshots = {}
    for unit in project.manifest['unit']:
        name = unit['unit']
        path = root / f'build/objdiff/base/{name}.obj'
        source = root / unit['source']
        flags = project.manifest['flags'][unit['flags']]
        key = (source.parent, tuple(flags))
        try:
            if key not in snapshots:
                snapshots[key] = compiled_freshness.snapshot(root, source, flags, includes, toolchain)
            expected = dict(snapshots[key], source=str(source.resolve()))
            # source itself is also an explicit file input, not just a tree.
            files = dict(expected['files'])
            files.pop(snapshots[key]['source'])
            files[str(source.resolve())] = compiled_freshness.digest(source)
            expected['files'] = files
            digest = compiled_freshness.validate(path, expected)
            raw = path.read_bytes()
            if hashlib.sha256(raw).hexdigest() != digest:
                raise ValueError(f'{name}: raw object changed during extraction')
            coff = CoffObject(raw)
            objects[name] = coff
            rows.extend(inventory(name, coff, digest))
        except (OSError, ValueError) as exc:
            issues.append(dict(unit=name, kind='unavailable-emitted-data', detail=str(exc)))
    for snapshot in snapshots.values():
        if snapshot != compiled_freshness.snapshot(root, Path(snapshot['source']), snapshot['flags'], includes, toolchain):
            raise ValueError('compiler inputs changed during candidate extraction; retry on stable inputs')
    bindings = bind(declared, rows, objects)
    from homm3.sema.retail_layout import Layout
    layout = Layout(project.image.data)
    sections = [s for s in layout.sections if s.name in ('.rdata', '.data', '.bss')]
    for binding in bindings:
        if binding['status'] != 'bound':
            continue
        start, size = binding['rva'], binding['size']
        if start is None or not any(s.rva <= start < start+size <= s.rva+s.mapped_size for s in sections):
            binding['status'] = 'outside-retail-data'
    spans = sorted((b['rva'], b['rva']+b['size']) for b in bindings if b['status'] == 'bound')
    bound_bytes, end = 0, 0
    for lo, hi in spans:
        bound_bytes += max(0, hi-max(lo, end))
        end = max(end, hi)
    implementations = ['analysis/candidate_data.py', 'analysis/data_declarations.py', 'analysis/data_symbols.py',
                       'build/compiled_freshness.py', 'build/canonicalize_data_symbols.py',
                       'core/project.py', 'core/cc_wrap.py', 'sema/retail_layout.py']
    inputs = ['scripts/homm3/'+p for p in implementations] + ['build/gen/data-declarations.json']
    for unit in objects:
        inputs.extend([f'build/objdiff/base/{unit}.obj', f'build/objdiff/base/{unit}.obj.compile.json'])
    return dict(schema=SCHEMA, candidate_data=rows, data_bindings=bindings, candidate_issues=issues,
                summary=dict(fresh_units=len(objects), unavailable_units=len(issues),
                             allocations=len(rows), candidate_physical_bytes=sum(r['physical_size'] for r in rows),
                             binding_statuses=dict(Counter(b['status'] for b in bindings)),
                             bound_retail_projection_bytes=bound_bytes,
                             compared_bytes=0, matched_bytes=0),
                declaration_fingerprint=declared['fingerprint'], retail_sha256=hashlib.sha256(layout.data).hexdigest(),
                input_sha256={p: compiled_freshness.digest(root/p) for p in inputs})


def export(report, directory):
    def render(row):
        return {key: '' if value is None else json.dumps(value, ensure_ascii=True)
                if isinstance(value, (list, dict)) or isinstance(value, str) and any(c in value for c in '\t\r\n')
                else hex(value) if key == 'rva' else value for key, value in row.items()}
    for name, key, header in [('candidate-data', 'candidate_data', ['id', 'unit', 'physical_size']),
                              ('data-bindings', 'data_bindings', ['id', 'rva', 'status']),
                              ('candidate-issues', 'candidate_issues', ['unit', 'kind', 'detail'])]:
        rows = report[key]
        tsv.write(directory / f'{name}.tsv', ['# Generated source/emission evidence; a binding is not a retail byte verdict.'],
                  list(rows[0]) if rows else header, [render(r) for r in rows])
    summary = {k: v for k, v in report.items() if k not in ('candidate_data', 'data_bindings', 'candidate_issues')}
    (directory / 'candidate-summary.json').write_text(json.dumps(summary, indent=2) + '\n')


def main(argv=None):
    from homm3.analysis import data_declarations
    from homm3.core.common import HOMM3_DIR
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HOMM3_DIR/'build/candidate-data')
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args(argv)
    declared = data_declarations.extract(HOMM3_DIR, Project(HOMM3_DIR).image.image_base, jobs=args.jobs)
    report = extract(HOMM3_DIR, declared)
    export(report, args.output)
    print(json.dumps(report['summary'], indent=2))
    return 1 if report['candidate_issues'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
