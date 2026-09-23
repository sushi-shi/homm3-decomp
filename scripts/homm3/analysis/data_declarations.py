"""Compiler-bound DATA declarations; source storage extents, never retail proof."""
from __future__ import annotations

from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile

from homm3.core import compiler_profile
from homm3.core.project import Project
from homm3.retail_labels import source

SCHEMA = 'homm3.data-declarations.v3'
SUFFIXES = {'.c', '.cpp', '.cxx', '.h', '.hpp', '.hxx'}


def annotation_sites(root, image_base):
    """Reuse the annotation scanner; cross-check sites, not just address counts."""
    sites = []
    for directory in ('src', 'include'):
        for path in sorted((root / directory).rglob('*')):
            if path.suffix not in SUFFIXES or path.name == 'va.h':
                continue
            raw = path.read_text(errors='replace')
            masked = source.mask_lexical_noise(raw)
            for macro in ('DATA', 'DATA_COMPGEN', 'DATA_COMPGEN_GUARD'):
                head, arity, _ = source.MACRO_HEADS[macro]
                for start, end, args, raw_args in source.macro_invocations(masked, head, raw):
                    prefix = masked[masked.rfind('\n', 0, start) + 1:start]
                    if re.fullmatch(r'\s*#\s*define\s+', prefix):
                        continue
                    valid = end is not None and len(args) == arity and source.ADDR_ARG_RE.fullmatch(args[0])
                    sites.append(dict(macro=macro, path=path.relative_to(root).as_posix(),
                                      line=raw.count('\n', 0, start) + 1, offset=len(raw[:start].encode('utf-8')),
                                      rva=int(args[0], 16) - image_base if valid else None,
                                      name=args[1] if valid and arity == 3 else '',
                                      expression=raw_args[2] if valid and arity == 3 else ''))
    return sites


def storage_extent(ty):
    """sizeof(reference) is the referent; a stored reference occupies a pointer."""
    import clang.cindex as cx
    canonical = ty.get_canonical()
    if canonical.kind in (cx.TypeKind.LVALUEREFERENCE, cx.TypeKind.RVALUEREFERENCE):
        # Profiles select the pinned i686 Windows ABI, not the host pointer size.
        return 4, 4, True
    size, alignment = canonical.get_size(), canonical.get_align()
    return size if size >= 0 else None, alignment if alignment >= 0 else None, False


def parse_unit(task):
    import clang.cindex as cx
    root = Path(task['root'])
    path = root / task['source']
    result = dict(unit=task['unit'], source=task['source'], facts=[], definitions=[],
                  errors=[], full_errors=[], skipped_bodies=False, active_macros=[], function_claims=[])
    try:
        index = cx.Index.create()
        options = cx.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD
        tu = index.parse(str(path), args=task['args'], options=options)
        errors = [str(d) for d in tu.diagnostics if d.severity >= cx.Diagnostic.Error]
        if errors:
            result['full_errors'] = errors
            tu = index.parse(str(path), args=task['args'],
                             options=options | cx.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
            result['skipped_bodies'] = True
            errors = [str(d) for d in tu.diagnostics if d.severity >= cx.Diagnostic.Error]
        result['errors'] = errors
        if errors:
            return result
    except (cx.LibclangError, cx.TranslationUnitLoadError) as exc:
        result['errors'] = [str(exc)]
        return result

    def location(cursor):
        loc = cursor.location
        return dict(path=Path(loc.file.name).relative_to(root).as_posix(),
                    line=loc.line, offset=loc.offset)

    functions = []
    def visit(node):
        if node.location.file:
            node_path = Path(node.location.file.name)
            if not any(node_path.is_relative_to(root / d) for d in ('src', 'include')):
                return
        children = list(node.get_children())
        if node.kind in (cx.CursorKind.FUNCTION_DECL, cx.CursorKind.CXX_METHOD,
                         cx.CursorKind.CONSTRUCTOR, cx.CursorKind.DESTRUCTOR) and node.location.file:
            functions.append(dict(path=location(node)['path'], start=node.extent.start.offset,
                                  end=node.extent.end.offset, symbol=node.mangled_name))
            for attribute in children:
                if attribute.kind != cx.CursorKind.ANNOTATE_ATTR:
                    continue
                claim = re.fullmatch(r'va:(0x[0-9a-fA-F]+) size:(0x[0-9a-fA-F]+|[0-9]+)', attribute.spelling)
                if claim:
                    result['function_claims'].append(dict(location(node), unit=task['unit'],
                        symbol=node.mangled_name, rva=int(claim[1], 0)-task['image_base'],
                        size=int(claim[2], 0), evidence='compiler-bound source VA annotation'))
        if node.kind == cx.CursorKind.MACRO_INSTANTIATION and node.spelling in (
                'DATA_COMPGEN', 'DATA_COMPGEN_GUARD'):
            result['active_macros'].append(dict(location(node), macro=node.spelling))
        if node.kind == cx.CursorKind.VAR_DECL and node.location.file:
            attributes = [c for c in children if c.kind == cx.CursorKind.ANNOTATE_ATTR and
                          re.fullmatch(r'data:0x[0-9a-fA-F]+', c.spelling)]
            storage = node.storage_class.name
            parent_kind = node.semantic_parent.kind
            local = parent_kind in (cx.CursorKind.FUNCTION_DECL, cx.CursorKind.CXX_METHOD,
                                    cx.CursorKind.CONSTRUCTOR, cx.CursorKind.DESTRUCTOR)
            static_storage = not local or storage in ('STATIC', 'EXTERN')
            if attributes or (node.is_definition() and static_storage):
                size, alignment, reference = storage_extent(node.type)
                fact = dict(location(node), usr=node.get_usr(), name=node.spelling,
                            symbol=node.mangled_name, type=node.type.spelling,
                            size=size, alignment=alignment, reference_cell=reference,
                            linkage=node.linkage.name, storage=storage,
                            static_storage=static_storage, definition=node.is_definition(),
                            local=local,
                            parent_symbol=node.semantic_parent.mangled_name if local else '',
                            unit=task['unit'], size_evidence='clang-i686-msvc-layout')
                if node.is_definition() and static_storage:
                    result['definitions'].append(fact)
                for attribute in attributes:
                    result['facts'].append(dict(fact, rva=int(attribute.spelling[5:], 16) - task['image_base'],
                                                annotation_path=location(attribute)['path'],
                                                annotation_line=attribute.location.line,
                                                annotation_offset=attribute.location.offset))
        for child in children:
            visit(child)
    visit(tu.cursor)
    for macro in result['active_macros']:
        owners = [f for f in functions if f['path'] == macro['path'] and
                  f['start'] <= macro['offset'] < f['end']]
        macro['function_symbol'] = min(owners, key=lambda f: f['end']-f['start'])['symbol'] if owners else ''
    return result


def summarize(units, sites):
    """Deduplicate header sites, retain disagreements and match unannotated definitions."""
    issues, grouped, definitions = [], defaultdict(list), defaultdict(list)
    for unit in units:
        if unit['errors']:
            issues.append(dict(kind='parse-failure', unit=unit['unit'], source=unit['source'],
                               rva=None, detail='\n'.join(unit['errors'])))
        elif unit['skipped_bodies']:
            issues.append(dict(kind='bodies-skipped', unit=unit['unit'], source=unit['source'],
                               rva=None, detail='\n'.join(unit['full_errors'])))
        for fact in unit['definitions']:
            definitions[fact['usr']].append(fact)
        for fact in unit['facts']:
            grouped[(fact['annotation_path'], fact['annotation_offset'], fact['rva'], fact['usr'])].append(fact)
    declarations, bound_sites = [], set()
    for key, facts in sorted(grouped.items()):
        path, offset, rva, usr = key
        bound_sites.add((path, offset, rva))
        fact = facts[0]
        # External declarations share an entity; internal/header statics retain
        # their source identity and candidate TU copies in the evidence lists.
        entity_defs = definitions.get(usr, []) if usr else []
        sizes = {f['size'] for f in [*facts, *entity_defs] if f['size'] is not None}
        conflict = len(sizes) > 1
        size = next(iter(sizes)) if len(sizes) == 1 else None
        identity = hashlib.sha256(json.dumps(key).encode()).hexdigest()[:16]
        row = dict(id=identity, rva=rva, size=size, end=rva + size if size is not None else None,
                   name=fact['name'], symbol=fact['symbol'], type=fact['type'], usr=usr,
                   types=sorted({f['type'] for f in facts}), symbols=sorted({f['symbol'] for f in facts}),
                   source=f"{path}:{fact['annotation_line']}", path=path, offset=offset,
                   definition=bool(entity_defs), definition_sources=sorted({f"{f['path']}:{f['line']}" for f in entity_defs}),
                   units=sorted({f['unit'] for f in facts}),
                   definition_units=sorted({f['unit'] for f in entity_defs}),
                   definitions=sorted(entity_defs, key=lambda f: (f['unit'], f['path'], f['offset'])),
                   local=fact['local'],
                   sizes=sorted(sizes), alignments=sorted({f['alignment'] for f in facts if f['alignment'] is not None}),
                   static_storage=all(f['static_storage'] for f in facts),
                   size_evidence='clang-i686-msvc-layout', reference_cell=fact['reference_cell'],
                   linkage=fact['linkage'], status='sized' if size and not conflict else
                   'conflicting-size' if conflict else 'unknown-size')
        if not row['static_storage']:
            row['status'] = 'automatic-storage'
        declarations.append(row)
        if row['status'] != 'sized':
            issues.append(dict(kind=row['status'], unit=','.join(row['units']), source=row['source'], rva=rva,
                               detail=f"{row['type']} {row['name']}; observed sizes={sorted(sizes)}"))
    for site in sites:
        if site['macro'] != 'DATA':
            issues.append(dict(kind='compgen-extent-unbound', source=f"{site['path']}:{site['line']}", unit='',
                               rva=site['rva'], detail=site['macro'] + ' is not a DATA VarDecl; no storage extent inferred'))
        elif (site['path'], site['offset'], site['rva']) not in bound_sites:
            issues.append(dict(kind='annotation-unbound', source=f"{site['path']}:{site['line']}", unit='',
                               rva=site['rva'], detail='DATA site not bound by a successful AST parse (possibly inactive preprocessor branch)'))
    return dict(schema=SCHEMA, declarations=declarations, issues=issues,
                units=[{k: v for k, v in u.items() if k not in ('facts', 'definitions')} for u in units],
                sites=sites, analysis_complete=not any(i['kind'] != 'compgen-extent-unbound' for i in issues))


def fingerprint(root, profiles, tasks):
    """No timestamp cache: include source trees, actual headers, profiles and parser."""
    import clang.cindex as cx
    paths = set()
    for directory in [root / 'src', root / 'include', root / 'vendor', *profiles.includes]:
        if directory.is_dir():
            paths.update(p for p in directory.rglob('*') if p.is_file())
    paths.update(root / p for p in ('config/project.toml', 'config/units.toml',
        'scripts/homm3/analysis/data_declarations.py', 'scripts/homm3/retail_labels/source.py',
        'scripts/homm3/core/compiler_profile.py', 'scripts/homm3/core/clang.py',
        'scripts/homm3/core/project.py'))
    digest = hashlib.sha256(json.dumps([SCHEMA, tasks, cx.Config.library_path, cx.Config.library_file], sort_keys=True).encode())
    for path in sorted(paths):
        digest.update(str(path).encode())
        digest.update(hashlib.sha256(path.read_bytes()).digest())
    return digest.hexdigest()


def extract(root, image_base, jobs=4):
    project = Project(root)
    profiles = compiler_profile.Profiles(project)
    tasks = [dict(root=str(root), source=u['source'], unit=u['unit'], image_base=image_base,
                  args=profiles.for_source(root / u['source'])) for u in project.manifest['unit']
             if (root / u['source']).is_relative_to(root / 'src')]
    key = fingerprint(root, profiles, tasks)
    dest = root / 'build/gen/data-declarations.json'
    if dest.is_file():
        try:
            cached = json.loads(dest.read_text())
            if cached.get('fingerprint') == key and cached.get('schema') == SCHEMA:
                return cached
        except (OSError, ValueError):
            pass
    if jobs == 1:
        units = [parse_unit(task) for task in tasks]
    else:
        with ProcessPoolExecutor(max_workers=jobs) as pool:
            units = list(pool.map(parse_unit, tasks))
    result = summarize(units, annotation_sites(root, image_base))
    result['fingerprint'] = key
    dest.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(dir=dest.parent, prefix='.data-declarations-')
    try:
        with os.fdopen(fd, 'w') as stream:
            json.dump(result, stream, indent=2)
            stream.write('\n')
        os.replace(name, dest)
    finally:
        Path(name).unlink(missing_ok=True)
    return result
