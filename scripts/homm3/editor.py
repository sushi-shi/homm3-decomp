"""Compile shared source and locate exact unrelocated bodies in pinned editors.

This is an initial discovery path, not an editor scoreboard. Functions with
relocations are reported as skipped: masking unresolved references cannot prove
an exact match. All objects and reports stay outside the game's build pipeline.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from homm3.build.canonicalize_data_symbols import CoffObject, _function_ranges
from homm3.core import common
from homm3.core.inputs import InputError
from homm3.core.project import Project

TARGETS = ('h3maped', 'h3ccmped')
DEFAULT_PROFILE = 'editor_o1_mt_gr_windows'


def inventory(path: Path, image) -> dict[int, int]:
    """Reviewed contiguous spans, separate from unreviewed discovery output."""
    from homm3.sema.data import read
    result = {}
    with path.open() as stream:
        rows = csv.DictReader((line for line in stream if not line.startswith('#')),
                              delimiter='\t')
        for row in rows:
            rva, size = int(row['rva'], 0), int(row['size'], 0)
            if rva in result:
                raise ValueError(f'{path}: duplicate RVA {rva:#x}')
            read(image, rva, size)
            if not image.section_of(rva).executable:
                raise ValueError(f'{path}: non-code RVA {rva:#x}')
            result[rva] = size
    end = 0
    for rva, size in sorted(result.items()):
        if rva < end:
            raise ValueError(f'{path}: overlapping span at {rva:#x}')
        end = rva + size
    return result


def scan(coff: CoffObject, image, spans: dict[int, int]) -> list[dict]:
    """Require whole bytes and an independently admitted extent for exactness."""
    rows = []
    sections = [(section.rva, image.blob(section))
                for section in image.sections if section.executable]
    for section, functions in _function_ranges(coff).items():
        body = coff.section_bytes(coff.sections[section - 1])
        for start, end, symbol in functions:
            raw = body[start:end]
            relocations = [r for r in coff.relocations if r.section == section
                           and start <= r.site < end]
            hits = []
            row = dict(symbol=symbol.name, section=section, offset=start,
                       size=len(raw), relocations=len(relocations), hits=hits)
            if relocations:
                row['status'] = 'needs-relocation-comparison'
            elif not raw:
                row['status'] = 'empty'
            else:
                for rva, data in sections:
                    offset = 0
                    while (offset := data.find(raw, offset)) != -1:
                        address = rva + offset
                        hits.append(dict(
                            rva=address, va=image.image_base + address,
                            whole_function=spans.get(address) == len(raw)))
                        offset += 1
                row['status'] = ('exact-bytes' if any(h['whole_function'] for h in hits)
                                 else 'byte-hit' if hits else 'different')
            rows.append(row)
    return rows


def compile_unit(project: Project, target: str, unit: str, profile: str) -> Path:
    units = {row['unit']: row for row in project.manifest['unit']}
    if unit not in units:
        raise ValueError(f'unknown source unit: {unit}')
    if profile not in project.manifest['flags']:
        raise ValueError(f'unknown compiler profile: {profile}')
    output = project.root / 'build/editor' / target / profile / (unit + '.obj')
    output.with_suffix('.json').unlink(missing_ok=True)
    env = dict(os.environ, HOMM3_DIR=str(project.root),
               PYTHONPATH=str(project.root / 'scripts'),
               WINEPREFIX=str(project.root / 'build/wineprefix'))
    result = subprocess.run([
        sys.executable, '-m', 'homm3.core.cc_wrap', '--out', str(output),
        '--src', str(project.root / units[unit]['source']), '--',
        *project.manifest['flags'][profile],
    ], cwd=project.root, env=env, stdout=sys.stderr)
    if result.returncode:
        # A failed compilation must never leave a reusable successful result.
        output.unlink(missing_ok=True)
        raise ValueError(f'VC6 compilation failed ({result.returncode})')
    return output


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog='homm3 editor', description=__doc__)
    parser.add_argument('target', choices=TARGETS)
    parser.add_argument('unit', help='existing source unit in config/units.toml')
    parser.add_argument('--profile', default=DEFAULT_PROFILE)
    parser.add_argument('--json', action='store_true')
    args = parser.parse_args(argv)
    try:
        project = Project(common.HOMM3_DIR)
        image = project.input_image(args.target)
        spans = inventory(project.root / 'config/retail' / args.target / 'functions.tsv', image)
        output = compile_unit(project, args.target, args.unit, args.profile)
        raw = output.read_bytes()
        rows = scan(CoffObject(raw), image, spans)
        result = dict(schema='homm3.editor.candidates.v1', exploratory=True,
                      target=args.target, image_sha256=hashlib.sha256(image.data).hexdigest(),
                      unit=args.unit, profile=args.profile,
                      flags=project.manifest['flags'][args.profile],
                      object=str(output), object_sha256=hashlib.sha256(raw).hexdigest(),
                      inventory_scope='reviewed seed spans, not a complete editor census',
                      functions=rows)
        report = output.with_suffix('.json')
        report.write_text(json.dumps(result, indent=2) + '\n')
        if args.json:
            print(json.dumps(result, indent=2))
        else:
            print(f'{args.target}: {args.unit} / {args.profile}')
            for row in rows:
                for hit in row['hits']:
                    if hit['whole_function']:
                        print(f"  {hit['va']:#010x}: {row['symbol']} ({row['size']} exact bytes)")
            exact = sum(row['status'] == 'exact-bytes' for row in rows)
            skipped = sum(row['status'] == 'needs-relocation-comparison' for row in rows)
            print(f'{exact} candidate bodies match reviewed spans; {skipped} need relocation comparison.')
            print(f'Unreviewed byte hits remain in the report; identity and duplicates need review.\nReport: {report}')
        return 0
    except (InputError, OSError, ValueError) as exc:
        print(f'[editor] {exc}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
