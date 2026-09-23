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

from homm3.analysis import data_body_recovery
from homm3.core import compiler_profile
from homm3.core.project import Project
from homm3.retail_labels import source

SCHEMA = 'homm3.data-declarations.v8'
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
    size = canonical.get_size()
    # libclang may crash in get_align for an incomplete array whose record
    # element is also incomplete. No object extent exists to align here.
    if size < 0:
        return None, None, False
    alignment = canonical.get_align()
    return size, alignment if alignment >= 0 else None, False


def type_shape(ty):
    """Compiler-derived dimensions and byte strides; never parse type spellings."""
    import clang.cindex as cx
    ty = ty.get_canonical()
    dimensions, strides = [], []
    while ty.kind in (cx.TypeKind.CONSTANTARRAY, cx.TypeKind.INCOMPLETEARRAY):
        count = ty.get_array_size()
        element = ty.get_array_element_type()
        size = element.get_size()
        dimensions.append(count if count >= 0 else None)
        strides.append(size if size >= 0 else None)
        ty = element.get_canonical()
    size = ty.get_size()
    pointee_size = None
    if ty.kind in (cx.TypeKind.POINTER, cx.TypeKind.LVALUEREFERENCE, cx.TypeKind.RVALUEREFERENCE):
        pointee = ty.get_pointee().get_size()
        pointee_size = pointee if pointee >= 0 else None
    return dict(dimensions=dimensions, strides_bytes=strides, element_type=ty.spelling,
                element_kind=ty.kind.name, element_bytes=size if size >= 0 else None,
                pointee_bytes=pointee_size, evidence='clang-i686 canonical type/layout; not semantic unit names')


def conflicting_shapes(left, right):
    # Canonical spelling can still omit default template arguments in another
    # TU. Spelling is retained as evidence, not used as a physical-shape verdict.
    for key in ('dimensions', 'strides_bytes', 'element_kind', 'element_bytes', 'pointee_bytes'):
        a, b = left[key], right[key]
        if isinstance(a, list) and isinstance(b, list):
            if len(a) != len(b) or any(x is not None and y is not None and x != y for x, y in zip(a, b)):
                return True
        elif a is not None and b is not None and a != b:
            return True
    return False


def source_language(source_path, args):
    """Retain the language selected by the manifest's translated driver flags."""
    language = 'c' if Path(source_path).suffix.lower() == '.c' else 'c++'
    for index, arg in enumerate(args):
        if arg in ('/TC', '-xc'):
            language = 'c'
        elif arg in ('/TP', '-xc++'):
            language = 'c++'
        elif arg == '-x' and index+1 < len(args):
            language = args[index+1]
    return language


def parse_unit(task):
    import clang.cindex as cx
    root = Path(task['root']).resolve()
    path = root / task['source']
    language = source_language(task['source'], task['args'])
    result = dict(unit=task['unit'], source=task['source'], facts=[], definitions=[],
                  storage_declarations=[], anonymous_namespace_files=[],
                  language=language, parse_mode='full', body_recovery={},
                  errors=[], full_errors=[], skipped_bodies=False, active_macros=[], function_claims=[])
    try:
        index = cx.Index.create()
        options = cx.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD
        args = [*task['args'], '-ferror-limit=0']
        tu = index.parse(str(path), args=args, options=options)
        errors = [str(d) for d in tu.diagnostics if d.severity >= cx.Diagnostic.Error]
        if errors:
            result['full_errors'] = errors
            recovered, result['body_recovery'] = data_body_recovery.recover(
                index, tu, path, args, options, lambda ty: (storage_extent(ty), type_shape(ty)))
            result['skipped_bodies'] = True
            result['parse_mode'] = 'isolated-bodies' if recovered is not None else 'all-bodies-skipped'
            tu = recovered if recovered is not None else index.parse(str(path), args=args,
                options=options | cx.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
            errors = [str(d) for d in tu.diagnostics if d.severity >= cx.Diagnostic.Error]
        result['errors'] = errors
        if errors:
            return result
    except (cx.LibclangError, cx.TranslationUnitLoadError) as exc:
        result['errors'] = [str(exc)]
        return result

    def location(cursor):
        loc = cursor.location
        return dict(path=Path(loc.file.name).resolve().relative_to(root).as_posix(),
                    line=loc.line, offset=loc.offset)

    functions = []
    def visit(node):
        if node.location.file:
            node_path = Path(node.location.file.name).resolve()
            if not any(node_path.is_relative_to(root / d) for d in ('src', 'include', 'vendor')):
                return
        children = list(node.get_children())
        if node.kind == cx.CursorKind.NAMESPACE and not node.spelling and node.location.file:
            result['anonymous_namespace_files'].append(location(node)['path'])
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
                        size=int(claim[2], 0), linkage=node.linkage.name,
                        evidence='compiler-bound source VA annotation'))
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
            if attributes or static_storage:
                # libclang does not call a C tentative definition a definition.
                # Its typed allocation request is useful, but does not prove
                # which strong/COMMON definition the linker ultimately chooses.
                tentative = (language == 'c' and not node.is_definition() and
                    parent_kind == cx.CursorKind.TRANSLATION_UNIT and storage in ('NONE', 'STATIC'))
                definition = node.is_definition() or tentative
                size, alignment, reference = storage_extent(node.type)
                referent = node.type.get_canonical().get_pointee() if reference else None
                const_array_reference = bool(referent is not None and
                    referent.kind == cx.TypeKind.CONSTANTARRAY and referent.is_const_qualified())
                fact = dict(location(node), usr=node.get_usr(), name=node.spelling,
                            symbol=node.mangled_name, type=node.type.spelling,
                            size=size, alignment=alignment, reference_cell=reference,
                            const_array_reference=const_array_reference,
                            shape=type_shape(node.type),
                            linkage=node.linkage.name, storage=storage,
                            static_storage=static_storage, definition=definition,
                            tentative_definition=tentative, language=language,
                            local=local,
                            parent_symbol=node.semantic_parent.mangled_name if local else '',
                            parent_name=node.semantic_parent.spelling if local else '',
                            unit=task['unit'], size_evidence='clang-i686-msvc-layout')
                if static_storage:
                    result['storage_declarations'].append(fact)
                if definition and static_storage:
                    result['definitions'].append(fact)
                for attribute in attributes:
                    result['facts'].append(dict(fact, rva=int(attribute.spelling[5:], 16) - task['image_base'],
                                                annotation_path=location(attribute)['path'],
                                                annotation_line=attribute.location.line,
                                                annotation_offset=attribute.location.offset))
        for child in children:
            visit(child)
    visit(tu.cursor)
    result['anonymous_namespace_files'] = sorted(set(result['anonymous_namespace_files']))
    for fact in result['facts']+result['definitions']+result['storage_declarations']:
        fact['anonymous_namespace_files'] = result['anonymous_namespace_files']
    for macro in result['active_macros']:
        owners = [f for f in functions if f['path'] == macro['path'] and
                  f['start'] <= macro['offset'] < f['end']]
        macro['function_symbol'] = min(owners, key=lambda f: f['end']-f['start'])['symbol'] if owners else ''
    return result


def summarize(units, sites):
    """Deduplicate header sites, retain disagreements and match unannotated definitions."""
    issues, grouped, definitions = [], defaultdict(list), defaultdict(list)
    declarations_by_usr = defaultdict(list)
    for unit in units:
        if unit['errors']:
            issues.append(dict(kind='parse-failure', unit=unit['unit'], source=unit['source'],
                               rva=None, detail='\n'.join(unit['errors'])))
        elif unit['skipped_bodies']:
            issues.append(dict(kind='bodies-skipped', unit=unit['unit'], source=unit['source'],
                               rva=None, detail=unit.get('parse_mode', 'all-bodies-skipped')+'; '+
                               '\n'.join(unit['full_errors'])))
        for fact in unit['definitions']:
            definitions[fact['usr']].append(fact)
        for fact in unit.get('storage_declarations', []):
            declarations_by_usr[fact['usr']].append(fact)
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
        observations = [*facts, *entity_defs, *declarations_by_usr.get(usr, [])]
        sizes = {f['size'] for f in observations if f['size'] is not None}
        shapes = {json.dumps(f['shape'], sort_keys=True) for f in observations if 'shape' in f}
        shape_rows = [json.loads(s) for s in sorted(shapes)]
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
                   local=fact['local'], parent_symbol=fact['parent_symbol'], parent_name=fact['parent_name'],
                   language=fact['language'], storage=fact['storage'],
                   sizes=sorted(sizes), alignments=sorted({f['alignment'] for f in facts if f['alignment'] is not None}),
                   shapes=shape_rows,
                   shape_conflict=any(conflicting_shapes(a, b) for i, a in enumerate(shape_rows) for b in shape_rows[i+1:]),
                   static_storage=all(f['static_storage'] for f in facts),
                   size_evidence='clang-i686-msvc-layout', reference_cell=fact['reference_cell'],
                   linkage=fact['linkage'], status='sized' if size and not conflict else
                   'conflicting-size' if conflict else 'unknown-size')
        if not row['static_storage']:
            row['status'] = 'automatic-storage'
        declarations.append(row)
        if row['shape_conflict']:
            issues.append(dict(kind='conflicting-shape', unit=','.join(row['units']), source=row['source'],
                               rva=rva, detail='same source entity has different canonical dimensions, strides or element types'))
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
                units=[{k: v for k, v in u.items() if k != 'facts'} for u in units],
                sites=sites, analysis_complete=not any(i['kind'] != 'compgen-extent-unbound' for i in issues))


def fingerprint(root, profiles, tasks):
    """No timestamp cache: include source trees, actual headers, profiles and parser."""
    import clang.cindex as cx
    paths = set()
    for directory in [root / 'src', root / 'include', root / 'vendor', *profiles.includes]:
        if directory.is_dir():
            paths.update(p for p in directory.rglob('*') if p.is_file())
    paths.update(root / p for p in ('config/project.toml', 'config/units.toml',
        'scripts/homm3/analysis/data_declarations.py', 'scripts/homm3/analysis/data_body_recovery.py',
        'scripts/homm3/retail_labels/source.py',
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
                  args=profiles.for_source(root / u['source'])) for u in project.manifest['unit']]
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
