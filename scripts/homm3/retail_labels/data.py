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
from homm3.core.images import path as _image_path


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


def _declarations(path: Path, profiles, *, bodies=False, reached=None):
    import clang.cindex as cx

    path = path.resolve()
    tu = cx.Index.create().parse(
        str(path), args=[*profiles.for_source(path), '-ferror-limit=0'],
        options=0 if bodies else cx.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
    if reached is not None:
        # Every file this parse read, so its cache entry tracks exactly them.
        reached.update(inclusion.include.name for inclusion in tu.get_includes())
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
        parent = cursor.semantic_parent
        if (cursor.storage_class == cx.StorageClass.STATIC
                and parent is not None and parent.kind == cx.CursorKind.NAMESPACE
                and not parent.spelling
                and parent.semantic_parent.kind == cx.CursorKind.TRANSLATION_UNIT):
            name = msvc_names.anonymous_static(cursor.spelling)
        else:
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


def _uncached_declarations(path: Path, profiles, reached=None):
    facts, errors = _declarations(path, profiles, reached=reached)
    if errors:
        return facts, errors
    requested = {int(m[1], 16) - profiles.project.image.image_base
                 for m in re.finditer(r'\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)',
                                      path.read_text())}
    if requested - facts.keys():
        local, local_errors = _declarations(path, profiles, bodies=True, reached=reached)
        facts.update(local)
        errors.extend(local_errors)
    return facts, errors


def data_rows(rows):
    """The fragment rows `enrich` attaches declaration facts to."""
    return [r for r in rows if r['kind'] == 'data' and r['channel'] == 'src-DATA']


def enrich(path, rows, profiles):
    """Attach typed facts to the existing source fragment, without a ledger."""
    data_rows_ = data_rows(rows)
    if not data_rows_:
        return []
    facts, errors = declarations(path, profiles)
    for row in data_rows_:
        fact = facts.get(row['rva'])
        if fact is None:
            errors.append(f'{path.name}: DATA({row["rva"]:#x}) has no typed declaration')
            continue
        row.update(size=fact['size'], joined=fact['name'],
                   type=fact['type'], defined='1' if fact['defined'] else '0',
                   source=fact['source'], internal='1' if fact['internal'] else '0')
    return errors


def _content_hash(profiles, file: str) -> str | None:
    """Command-scoped content hash of one dependency (None when unreadable)."""
    hashes = getattr(profiles, '_data_dependency_hashes', None)
    if hashes is None:
        hashes = profiles._data_dependency_hashes = {}
    if file not in hashes:
        try:
            hashes[file] = hashlib.sha256(Path(file).read_bytes()).hexdigest()
        except OSError:
            hashes[file] = None
    return hashes[file]


def _shared_fingerprint(profiles) -> str:
    root = profiles.project.root
    shared = getattr(profiles, '_data_fingerprint', None)
    if shared is None:
        digest = hashlib.sha256(Path(__file__).read_bytes() + Path(msvc_names.__file__).read_bytes())
        digest.update(b'dependency-scoped\0')
        units = root / _image_path('config/units.toml')
        if units.is_file():
            digest.update(str(units.relative_to(root)).encode())
            digest.update(units.read_bytes())
        for directory in ('include', 'vendor', _image_path('build/gen/msvc-include')):
            for file in sorted(p for p in (root / directory).rglob('*') if p.is_file()):
                digest.update(str(file.relative_to(root)).encode())
                if directory.startswith('build/'):
                    digest.update(file.read_bytes())
        shared = digest.hexdigest()
        profiles._data_fingerprint = shared
    return shared


def _lookup(path: Path, profiles):
    """(cache file, fingerprint, cached (facts, errors) or None)."""
    fingerprint = hashlib.sha256((_shared_fingerprint(profiles)
                                  + repr(profiles.for_source(path))).encode()
                                 + path.read_bytes()).hexdigest()
    cache = profiles.project.root / 'build/cache/data-declarations' / (path.stem + '.json')
    try:
        saved = json.loads(cache.read_text())
        if (saved['fingerprint'] == fingerprint
                and all(digest is not None and _content_hash(profiles, file) == digest
                        for file, digest in saved['dependencies'].items())):
            return cache, fingerprint, ({int(k): v for k, v in saved['facts'].items()},
                                        saved['errors'])
    except (OSError, ValueError, KeyError, TypeError, AttributeError):
        pass
    return cache, fingerprint, None


def _store(cache: Path, fingerprint: str, profiles, reached, facts, errors) -> None:
    dependencies = {file: _content_hash(profiles, file) for file in sorted(reached)}
    cache.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(mode='w', dir=cache.parent, delete=False) as f:
        json.dump(dict(fingerprint=fingerprint, dependencies=dependencies,
                       facts=facts, errors=errors), f)
        temporary = f.name
    os.replace(temporary, cache)


def declarations(path: Path, profiles):
    """Cache a parse by source, profile, implementation and dependency contents.

    Each entry records the content of every file its parse read, so a header
    edit reparses only the TUs that include it. The shared key holds the
    implementation, units.toml, the mirrored standard headers and the NAMES
    of all project/vendor headers: adding or removing one can change include
    resolution, so it reparses every TU. Profiles is command-scoped, so its
    shared digest and dependency hashes are safe to reuse between that
    command's concurrent TU parses. Errors are cached too.
    """
    cache, fingerprint, hit = _lookup(path, profiles)
    if hit is not None:
        return hit
    reached = set()
    facts, errors = _uncached_declarations(path, profiles, reached)
    _store(cache, fingerprint, profiles, reached, facts, errors)
    return facts, errors


_PRIME_PROFILES = None


def _prime_one(path_text: str):
    reached = set()
    facts, errors = _uncached_declarations(Path(path_text), _PRIME_PROFILES, reached)
    return facts, errors, sorted(reached)


def prime(paths, profiles, jobs: int | None = None) -> int:
    """Parse the stale entries among `paths` in forked worker processes.

    The cursor walk is Python-bound, so concurrent threads serialize on the
    GIL. Workers inherit these profiles and the parent writes exactly the
    entries `declarations` would. Returns the number of entries refreshed.
    """
    global _PRIME_PROFILES
    stale = []
    for path in paths:
        cache, fingerprint, hit = _lookup(path, profiles)
        if hit is None:
            stale.append((path, cache, fingerprint))
    workers = min(jobs or min(8, os.cpu_count() or 1), len(stale))
    if workers < 2:
        return 0  # a single parse gains nothing from a worker
    import multiprocessing
    from concurrent.futures import ProcessPoolExecutor
    _PRIME_PROFILES = profiles
    try:
        with ProcessPoolExecutor(max_workers=workers,
                                 mp_context=multiprocessing.get_context('fork')) as pool:
            parsed = list(pool.map(_prime_one, [str(path) for path, _c, _f in stale]))
    finally:
        _PRIME_PROFILES = None
    for (_path, cache, fingerprint), (facts, errors, reached) in zip(stale, parsed):
        _store(cache, fingerprint, profiles, reached, facts, errors)
    return len(stale)
