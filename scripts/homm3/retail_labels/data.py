"""Gruntz DATA extraction with HoMM3 annotations and the manifest's VC6 ABI.

The source declaration supplies the extent, linkage and identity. No gap to
the next label and no equal retail payload supplies a missing declaration.
Objects are used later by the manifest to establish emission and topology.
"""
from __future__ import annotations

from pathlib import Path
import re
import hashlib
import json
import os
import tempfile

from homm3.core import msvc_names


def _error_bodies(tu, path, diagnostics):
    """Locate errors wholly inside source function bodies, or reject the TU.

    Header, signature and file-scope errors can invalidate types everywhere.
    An error inside an unrelated body does not invalidate a clean function's
    explicit local array type. Never recover declarations from that bad body.
    """
    import clang.cindex as cx

    callable_kinds = {cx.CursorKind.FUNCTION_DECL, cx.CursorKind.CXX_METHOD,
                      cx.CursorKind.CONSTRUCTOR, cx.CursorKind.DESTRUCTOR,
                      cx.CursorKind.CONVERSION_FUNCTION, cx.CursorKind.FUNCTION_TEMPLATE}
    bodies = []
    pending = list(tu.cursor.get_children())
    while pending:
        cursor = pending.pop()
        if not cursor.location.file or Path(cursor.location.file.name).resolve() != path:
            continue
        children = list(cursor.get_children())
        pending.extend(children)
        if cursor.kind in callable_kinds:
            bodies.extend((c.extent.start.offset, c.extent.end.offset)
                          for c in children if c.kind == cx.CursorKind.COMPOUND_STMT)
    bad = set()
    for diagnostic in diagnostics:
        location = diagnostic.location
        if not location.file or Path(location.file.name).resolve() != path:
            return None
        owners = [(start, end) for start, end in bodies
                  if start <= location.offset < end]
        if not owners:
            return None
        bad.update(owners)
    return bad


def _declarations(path: Path, profiles, *, bodies=False):
    import clang.cindex as cx

    path = path.resolve()
    tu = cx.Index.create().parse(
        str(path), args=[*profiles.for_source(path), '-ferror-limit=0'],
        options=0 if bodies else cx.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
    diagnostics = [d for d in tu.diagnostics if d.severity >= cx.Diagnostic.Error]
    errors = [str(d) for d in diagnostics]
    bad_bodies = _error_bodies(tu, path, diagnostics) if bodies else None
    if errors and (not bodies or bad_bodies is None):
        return {}, errors
    facts = {}
    pending = list(tu.cursor.get_children())
    while pending:
        cursor = pending.pop()
        if not cursor.location.file or Path(cursor.location.file.name).resolve() != path:
            continue
        pending.extend(cursor.get_children())
        if cursor.kind != cx.CursorKind.VAR_DECL or not cursor.location.file:
            continue
        if any(start <= cursor.location.offset < end for start, end in bad_bodies or ()):
            continue
        annotations = [child.spelling for child in cursor.get_children()
                       if child.kind == cx.CursorKind.ANNOTATE_ATTR]
        addresses = [int(match[1], 16) - profiles.project.image.image_base
                     for text in annotations
                     if (match := re.fullmatch(r'data:(0x[0-9a-fA-F]+)', text))]
        if not addresses:
            continue
        ty = cursor.type.get_canonical()
        # A stored reference occupies a pointer, not sizeof(referred object).
        size = (4 if ty.kind in (cx.TypeKind.LVALUEREFERENCE,
                                 cx.TypeKind.RVALUEREFERENCE) else ty.get_size())
        if size < 0 and not cursor.is_definition():
            size = 0
        if size < 0 or not cursor.mangled_name:
            errors.append(f'{cursor.location}: DATA declaration has no complete type/name')
            continue
        name = msvc_names.data(cursor.mangled_name, decorated=True,
                               internal=cursor.linkage != cx.LinkageKind.EXTERNAL)
        fact = dict(name=name, size=size, type=cursor.type.spelling,
                    defined=cursor.is_definition(),
                    internal=cursor.linkage != cx.LinkageKind.EXTERNAL,
                    source=f'{path.relative_to(profiles.project.root)}:{cursor.location.line}')
        for address in addresses:
            prior = facts.get(address)
            if prior is not None and any(prior[k] != fact[k] for k in ('name', 'size', 'type')):
                errors.append(f'{cursor.location}: conflicting DATA declarations at {address:#x}')
            else:
                fact['defined'] |= bool(prior and prior['defined'])
                facts[address] = dict(fact)
    return facts, errors


def _uncached_declarations(path: Path, profiles):
    facts, errors = _declarations(path, profiles)
    if errors:
        return facts, errors
    requested = {int(m[1], 16) - profiles.project.image.image_base
                 for m in re.finditer(r'\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)',
                                      path.read_text())}
    if requested - facts.keys():
        local, local_errors = _declarations(path, profiles, bodies=True)
        facts.update(local)
        errors.extend(local_errors)
    return facts, errors


def enrich(path, rows, profiles):
    """Attach typed facts to the existing source fragment, without a ledger."""
    data_rows = [r for r in rows if r['kind'] == 'data' and r['channel'] == 'src-DATA']
    if not data_rows:
        return []
    facts, errors = declarations(path, profiles)
    for row in data_rows:
        fact = facts.get(row['rva'])
        if fact is None:
            errors.append(f'{path.name}: DATA({row["rva"]:#x}) has no typed declaration')
            continue
        row.update(size=fact['size'], joined=fact['name'],
                   type=fact['type'], defined='1' if fact['defined'] else '0',
                   source=fact['source'], internal='1' if fact['internal'] else '0')
    return errors


def declarations(path: Path, profiles):
    """Cache a parse by source, header, profile and implementation contents.

    Profiles is command-scoped, so its shared header digest is safe to reuse
    between that command's concurrent TU parses. Errors are cached too.
    """
    root = profiles.project.root
    shared = getattr(profiles, '_data_fingerprint', None)
    if shared is None:
        digest = hashlib.sha256(Path(__file__).read_bytes() + Path(msvc_names.__file__).read_bytes())
        paths = [root / 'config/units.toml']
        for directory in ('include', 'vendor', 'build/gen/msvc-include'):
            paths.extend(sorted(p for p in (root / directory).rglob('*') if p.is_file()))
        for file in paths:
            if file.is_file():
                digest.update(str(file.relative_to(root)).encode())
                digest.update(file.read_bytes())
        shared = digest.hexdigest()
        profiles._data_fingerprint = shared
    fingerprint = hashlib.sha256((shared + repr(profiles.for_source(path))).encode()
                                 + path.read_bytes()).hexdigest()
    cache = root / 'build/cache/data-declarations' / (path.stem + '.json')
    try:
        saved = json.loads(cache.read_text())
        if saved['fingerprint'] == fingerprint:
            return {int(k): v for k, v in saved['facts'].items()}, saved['errors']
    except (OSError, ValueError, KeyError):
        pass
    facts, errors = _uncached_declarations(path, profiles)
    cache.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(mode='w', dir=cache.parent, delete=False) as f:
        json.dump(dict(fingerprint=fingerprint, facts=facts, errors=errors), f)
        temporary = f.name
    os.replace(temporary, cache)
    return facts, errors
