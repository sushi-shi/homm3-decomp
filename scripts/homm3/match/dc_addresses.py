"""Verify DC_ADDRESS against embedded NB11 and census the source migration."""
from collections import defaultdict
import re

from homm3.match import source_ownership as ownership


def validate(definitions, origins, rows, symbols, *, require_complete=False):
    """Check every claim, including reviewed Windows-specific interfaces.

    Inventory matches already check typed identities and overloads. A platform
    interface may differ, but cannot claim a different original function name.
    Completeness is the temporary migration gate; symbol verification remains.
    """
    errors = []
    paired = defaultdict(set)
    for row in rows:
        if row['status'] == 'matched' and int(row['dc_offset'], 16) in symbols.procedures:
            key = row['source_file'], row['source_line'], row['source_name'], row['signature']
            paired[key].add(int(row['dc_offset'], 16))
    owners = defaultdict(set)
    for d in definitions:
        where = f'{d.file}:{d.line} {d.name}'
        expected = paired[(d.file, d.line, d.name, d.signature)]
        claimed = set()
        for offset, size in d.dc_addresses:
            if offset in claimed:
                errors.append(f'DC_ADDRESS {where}: duplicate claim {offset:#x}')
            claimed.add(offset)
            proc = symbols.procedures.get(offset)
            if proc is None:
                errors.append(f'DC_ADDRESS {where}: {offset:#x} is not a debug procedure entry')
                continue
            if size != proc.size:
                errors.append(f'DC_ADDRESS {where}: {offset:#x} size {size:#x} != debug size {proc.size:#x}')
            if offset not in expected:
                name = proc.name
                if name.startswith("`anonymous namespace'::") and '?A0x' in d.mangled:
                    name = name.removeprefix("`anonymous namespace'::")
                if ownership.procedure_name(name) != ownership.procedure_name(d.original_name or d.name):
                    errors.append(f'DC_ADDRESS {where}: {offset:#x} names {proc.name}')
                elif expected:
                    errors.append(f'DC_ADDRESS {where}: {offset:#x} is a different debug overload/emission')
            owners[offset].add((d.file, d.offset, d.name, d.signature))
        if require_complete:
            for offset in sorted(expected - claimed):
                errors.append(f'DC_ADDRESS MISSING {where}: debug procedure {offset:#x} '
                              f'{symbols.procedures[offset].name}')
    for offset, bodies in sorted(owners.items()):
        if len(bodies) > 1:
            errors.append(f'DC_ADDRESS {offset:#x}: multiple authored owners: '
                          + ', '.join(f'{f} {n}' for f, _, n, _ in sorted(bodies)))
    return errors


def validate_source_sites(root, definitions):
    """No orphan, inactive or malformed source claim may escape the AST gate."""
    from homm3.analysis.dc_claims import claims
    from homm3.retail_labels.source import mask_lexical_noise
    by_file = defaultdict(list)
    for d in definitions:
        by_file[d.file].append(d)
    errors = []
    for directory in ('src', 'include'):
        for path in sorted((root / directory).rglob('*')):
            if path.suffix.lower() not in {'.h', '.hpp', '.inl', '.c', '.cpp', '.cxx'}:
                continue
            from homm3.core import images
            if images.foreign(path, root):
                continue
            text = path.read_text()
            masked = mask_lexical_noise(text)
            relative = path.relative_to(root).as_posix()
            parsed = {c.start: c for c in claims(text)}
            for match in re.finditer(r'\bDC_ADDRESS\s*\(', masked):
                start = match.start()
                if masked[masked.rfind('\n', 0, start) + 1:start].lstrip().startswith('#'):
                    continue
                where = f'{relative}:{text.count(chr(10), 0, start) + 1}'
                claim = parsed.get(start)
                if claim is None:
                    errors.append(f'DC_ADDRESS {where}: expected literal offset and procedure size')
                elif not any(d.offset <= start < d.end
                             and [claim.offset, claim.size] in [list(c) for c in d.dc_addresses]
                             for d in by_file[relative]):
                    errors.append(f'DC_ADDRESS {where}: claim has no active authored definition')
    return errors
