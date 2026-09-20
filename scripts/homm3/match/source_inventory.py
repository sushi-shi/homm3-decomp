"""Reconcile every DC procedure with active authored C++ in both directions.

This is source identity coverage, independent of byte-match scores. The only
exemptions are exact, reviewed dc_only.tsv / win_only.tsv rows. Library and
compiler-generated procedures remain visible until explicitly accounted for.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import csv
import json
from pathlib import Path

from homm3.core import common, inputs
from homm3.core.project import Project
from homm3.match import source_ownership as ownership

COLUMNS = ('module', 'status', 'dc_offset', 'dc_file', 'dc_line', 'dc_name',
           'source_file', 'source_line', 'source_name', 'signature', 'reason')
UNRESOLVED = {'missing_source', 'missing_dc', 'invalid_source'}


def origin_identity(origin):
    """Repeated emissions share a written identity; overloads do not."""
    return (origin.file, origin.name, origin.line,
            tuple(origin.argument_types) if origin.argument_types is not None else None,
            origin.const, origin.generated, origin.declaration_only, origin.type_index)


def definition_identity(definition):
    return definition.file, definition.offset, definition.name, definition.signature


def module_name(value):
    return value.removesuffix('.obj')


def reconcile(definitions, origins, dc_only, win_only, dc_inlined=None, *, symbols=None):
    """Return evidence rows and hard errors; never infer an exemption from absence."""
    dc_inlined = dc_inlined or {}
    matches = []
    errors, _ = ownership.compare(definitions, origins, dc_only, win_only, dc_inlined,
                                  symbols=symbols, matched_out=matches, strict_names=True)
    paired = defaultdict(list)
    matched_definitions = set()
    definitions_by_name = defaultdict(list)
    for definition in definitions:
        definitions_by_name[ownership.procedure_name(
            definition.original_name or definition.name)].append(definition)
    origin_names = {ownership.procedure_name(o.name) for o in origins}
    origin_offsets = {o.offset for o in origins if o.offset}
    for definition, counterparts in matches:
        if definition.template:
            # One authored template produces concrete instances in several
            # compilands. A DC offset can select one instance but does not
            # remove the others at that same proven source definition.
            locations = {(o.file, o.line, ownership.procedure_name(o.name))
                         for o in counterparts if not o.declaration_only}
            counterparts = tuple(o for o in origins
                if (o.file, o.line, ownership.procedure_name(o.name)) in locations
                and o.const == definition.const and not o.generated
                and o.argument_types is not None
                and len(o.argument_types) == len(definition.argument_types)
                and (o.file, o.name, str(o.line)) not in dc_only)
        if any(not o.declaration_only for o in counterparts):
            matched_definitions.add(definition_identity(definition))
        for counterpart in counterparts:
            paired[origin_identity(counterpart)].append(definition)

    # An exemption for a still-present, same-interface definition is stale.
    # Paired platform changes can legitimately use both tables, but an exact
    # declaration cannot be hidden by putting it in both exclusion lists.
    for origin in origins:
        key = origin.file, origin.name, str(origin.line)
        if key not in dc_only:
            continue
        for definition in definitions_by_name[ownership.procedure_name(origin.name)]:
            if ownership.source_file(definition.file.split('/', 1)[-1]) != origin.file:
                continue  # A platform shim can replace a same-signature external library API.
            arguments = tuple(definition.argument_types) + (('...',) if definition.variadic else ())
            if (origin.argument_types is not None
                    and tuple(map(ownership.type_identity, origin.argument_types))
                    == tuple(map(ownership.type_identity, arguments))
                    and origin.const == definition.const
                    and ownership.type_identity(origin.return_type)
                    == ownership.type_identity(definition.return_type)):
                errors.append(f'FILTER stale dc_only.tsv entry {key}: '
                              f'active counterpart {definition.file}:{definition.line} {definition.name}')

    # Positively recovered DC inline bodies have a source-row address rather
    # than a procedure. Keep them visible, with their actual header owner.
    all_origins = {origin_identity(o): o for o in origins}
    procedure_keys = set(all_origins)
    for _, counterparts in matches:
        for o in counterparts:
            all_origins.setdefault(origin_identity(o), o)
    rows = []
    # Retain repeated procedure emissions even though they share one source
    # identity. Recovered inline identities contribute one additional row.
    extra_origins = [o for key, o in all_origins.items() if key not in procedure_keys]
    for origin in [*origins, *extra_origins]:
        if origin.declaration_only:
            continue  # This inventory is procedure bodies, not un-emitted declarations.
        key = origin.file, origin.name, str(origin.line)
        counterparts = {definition_identity(d): d for d in paired[origin_identity(origin)]}
        row = dict.fromkeys(COLUMNS, '')
        row.update(module=module_name(origin.module), dc_offset=origin.offset,
                   dc_file=origin.file, dc_line=origin.line, dc_name=origin.name)
        if key in dc_only:
            row.update(status='documented_dc_only', reason=dc_only[key])
        elif len(counterparts) == 1:
            definition = next(iter(counterparts.values()))
            row.update(status='matched', source_file=definition.file,
                       source_line=definition.line, source_name=definition.name,
                       signature=definition.signature)
        else:
            row.update(status='missing_source', reason='No verified active source counterpart')
        rows.append(row)

    for definition in definitions:
        if definition_identity(definition) in matched_definitions:
            continue
        key = definition.file, definition.name, definition.signature
        row = dict.fromkeys(COLUMNS, '')
        # Shared headers retain their physical owner instead of inventing a
        # retail carrier. DC-matched header entries are listed in their DC TUs.
        row.update(module=(Path(definition.file).stem if definition.file.startswith('src/')
                           else definition.file), source_file=definition.file,
                   source_line=definition.line, source_name=definition.name,
                   signature=definition.signature)
        if key in dc_inlined:
            row.update(status='documented_dc_inlined', reason=dc_inlined[key])
        elif key in win_only:
            row.update(status='documented_win_only', reason=win_only[key])
        else:
            has_identity = (ownership.procedure_name(definition.original_name or definition.name)
                            in origin_names or definition.dc_offset in origin_offsets)
            row.update(status='invalid_source' if has_identity else 'missing_dc',
                       reason='No verified DC counterpart or reviewed win_only.tsv disposition')
        rows.append(row)
    return sorted(rows, key=lambda r: (r['module'], r['dc_file'], r['dc_line'] or 0,
                                      r['source_file'], r['source_line'] or 0)), errors


def validate_dc_roster(origins, symbols):
    """A stale/truncated generated CSV cannot erase raw CodeView procedures."""
    procedures = [o for o in origins if not o.declaration_only]
    by_offset = defaultdict(list)
    for origin in procedures:
        by_offset[int(origin.offset, 16)].append(origin)
    errors = []
    for offset in sorted(set(symbols.procedures) - by_offset.keys()):
        errors.append(f'DC-ROSTER missing raw procedure {offset:#x} '
                      f'{symbols.procedures[offset].name}; regenerate the DC corpus')
    for offset, rows in by_offset.items():
        expected = symbols.procedures.get(offset)
        if len(rows) != 1 or expected is None:
            errors.append(f'DC-ROSTER invalid/duplicate procedure {offset:#x}')
        elif rows[0].name != expected.name or rows[0].module != expected.module:
            errors.append(f'DC-ROSTER stale identity at {offset:#x}: expected '
                          f'{expected.module} {expected.name}')
    return errors


def audit(root=common.HOMM3_DIR, *, modules=(), jobs=4, fresh=False, origins=None):
    project = Project(root)
    definitions, errors, _ = ownership.collect(root, jobs, fresh)
    collection_errors = set(errors)
    errors.extend(ownership.active_stub_definitions(definitions, root))
    if origins is None:
        origins = ownership.read_dc(root, include_declarations=True, project=project)
    symbols = inputs.dreamcast_symbols(project)
    errors.extend(validate_dc_roster(origins, symbols))
    dc_only, failures = ownership.read_filter(root / 'config/dc_only.tsv', ('file', 'function', 'line'))
    errors.extend(failures)
    win_only, failures = ownership.read_filter(root / 'config/win_only.tsv', ('file', 'function', 'signature'))
    errors.extend(failures)
    dc_inlined, failures = ownership.read_filter(
        root / 'config/dc-inlined-helpers.tsv', ('file', 'function', 'signature'))
    errors.extend(failures)
    rows, failures = reconcile(definitions, origins, dc_only, win_only, dc_inlined,
                              symbols=symbols if any(d.inline_origin for d in definitions) else None)
    errors.extend(failures)
    if modules:
        selected = {module_name(m) for m in modules}
        known = {row['module'] for row in rows}
        for missing in sorted(selected - known):
            errors.append(f'MODULE unknown inventory module {missing}')
        rows = [row for row in rows if row['module'] in selected]
        files = {row['source_file'] for row in rows if row['source_file']}
        # Collection failures and exact filter integrity are global: an
        # incomplete AST scan cannot certify even an exempt module. Other
        # definition diagnostics are scoped
        # to the selected source/headers; no missing procedure is discarded.
        errors = [error for error in errors if error in collection_errors
                  or error.startswith(('FILTER ', 'MODULE ', 'DC-ROSTER '))
                  or any(f'{file}:' in error for file in files)
                  or any(f'src/{m}.cpp' in error for m in selected)]
    counts = dict(Counter(row['status'] for row in rows))
    missing = sum(counts.get(status, 0) for status in UNRESOLVED)
    return dict(complete=not errors and missing == 0, counts=counts,
                unresolved=missing, violations=errors, rows=rows)


def write_tsv(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('w', newline='') as stream:
        writer = csv.DictWriter(stream, COLUMNS, delimiter='\t', lineterminator='\n')
        writer.writeheader()
        writer.writerows(rows)


def run_gate(*, origins=None):
    result = audit(origins=origins)
    directory = common.HOMM3_DIR / 'build/reconciliation'
    write_tsv(directory / 'inventory.tsv', result['rows'])
    (directory / 'inventory.json').write_text(json.dumps(result, indent=2) + '\n')
    print(f'[build] source-inventory: {result["unresolved"]} unresolved entries; '
          f'{len(result["violations"])} violations; full comparison build/reconciliation/inventory.tsv')
    failures = list(result['violations'])
    if result['unresolved']:
        failures.append(f'SOURCE-INVENTORY {result["unresolved"]} unexplained source/DC entries; '
                        'see build/reconciliation/inventory.tsv')
    return failures


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module', action='append', default=[], help='DC compiland/source TU; repeatable')
    parser.add_argument('--json', action='store_true')
    parser.add_argument('--tsv', type=Path, help='write the complete comparison, including matched rows')
    parser.add_argument('--fresh', action='store_true')
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args(argv)
    result = audit(modules=args.module, jobs=args.jobs, fresh=args.fresh)
    if args.tsv:
        write_tsv(args.tsv, result['rows'])
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        by_module = defaultdict(Counter)
        for row in result['rows']:
            by_module[row['module']][row['status']] += 1
        for module, counts in sorted(by_module.items()):
            missing = sum(counts[status] for status in UNRESOLVED)
            print(f'{module}: {counts["matched"]} matched, '
                  f'{counts["documented_dc_only"]} documented DC-only, '
                  f'{counts["documented_win_only"]} documented Windows-only, {missing} unresolved')
        for error in result['violations']:
            print(error)
        print(f'Source inventory: {"COMPLETE" if result["complete"] else "INCOMPLETE"}; '
              f'{result["unresolved"]} unresolved entries, {len(result["violations"])} violations')
        if args.tsv:
            print(f'Full comparison: {args.tsv}')
    return 0 if result['complete'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
