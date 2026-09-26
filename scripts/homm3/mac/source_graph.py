"""Compiler-resolved authored calls; never a byte-match or inlining verdict."""
from __future__ import annotations

from collections import defaultdict, deque
import hashlib
import json
from pathlib import Path
import re
import sys

SCHEMA = 1


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def cache_valid(saved, key):
    if saved.get('key') != key or not saved.get('inputs'):
        return False
    try:
        return all(digest(path) == checksum for path, checksum in saved['inputs'].items())
    except OSError:
        return False


def scan(ci, source: Path, args: list[str], root: Path) -> dict:
    """Keep exact declaration USRs, including overload and const distinctions."""
    k = ci.CursorKind
    functions = {k.FUNCTION_DECL, k.CXX_METHOD, k.CONSTRUCTOR, k.DESTRUCTOR,
                 k.CONVERSION_FUNCTION, k.FUNCTION_TEMPLATE}
    tu = ci.Index.create().parse(str(source), args=args)
    nodes, edges, gaps = {}, [], []
    texts = {}

    def location(cursor):
        loc = cursor.location
        path = Path(loc.file.name).resolve() if loc.file else None
        return {'file': path.relative_to(root).as_posix() if path and path.is_relative_to(root)
                else str(path) if path else None, 'line': loc.line, 'offset': loc.offset}

    def project(cursor):
        file = location(cursor)['file'] or ''
        return file.startswith(('src/', 'include/'))

    def identity(cursor):
        cursor = cursor.canonical
        usr = cursor.get_usr()
        # File-static declarations must not merge across equal basenames.
        if usr and cursor.linkage == ci.LinkageKind.INTERNAL:
            usr = (location(cursor)['file'] or '') + ':' + usr
        return usr

    def node(cursor):
        usr = identity(cursor)
        if not usr:
            return None
        loc = location(cursor)
        scope, parent = [], cursor.semantic_parent
        while parent and parent.kind != k.TRANSLATION_UNIT:
            if parent.spelling:
                scope.append(parent.spelling)
            parent = parent.semantic_parent
        name = '::'.join([*reversed(scope), cursor.spelling])
        annotations = [c.spelling for c in cursor.get_children() if c.kind == k.ANNOTATE_ATTR]
        item = nodes.setdefault(usr, {'id': usr, 'name': name, 'type': cursor.type.spelling,
            'mangled': cursor.mangled_name, 'location': loc, 'definition': False,
            'mac': [], 'windows': [], 'project': project(cursor)})
        for annotation in annotations:
            match = re.fullmatch(r'(mac|va):(0x[0-9a-fA-F]+|\d+) size:(0x[0-9a-fA-F]+|\d+)', annotation)
            if match:
                field = 'mac' if match[1] == 'mac' else 'windows'
                claim = [int(match[2], 0), int(match[3], 0)]
                if claim not in item[field]:
                    item[field].append(claim)
        if cursor.is_definition():
            item['definition'] = True
            item['location'] = loc
        return usr

    def expression(cursor):
        extent = cursor.extent
        if not extent.start.file or extent.start.file != extent.end.file:
            return None
        file = extent.start.file.name
        if file not in texts:
            texts[file] = Path(file).read_bytes()
        return texts[file][extent.start.offset:extent.end.offset].decode(errors='replace')

    def body(cursor, owner):
        if cursor.kind in functions or cursor.kind == k.LAMBDA_EXPR:
            gaps.append({'caller': owner, 'kind': 'nested_callable', 'location': location(cursor)})
            return
        if cursor.kind == k.CALL_EXPR:
            target = cursor.referenced
            resolved = target is not None and target.kind in functions
            target_id = node(target) if resolved else None
            edge = {'caller': owner, 'callee': target_id, 'location': location(cursor),
                    'expression': expression(cursor), 'kind': 'call', 'dispatch': 'direct'}
            if resolved and target.kind == k.CXX_METHOD and target.is_virtual_method():
                # The declared method is known; the dynamic destination is not.
                edge['dispatch'] = 'virtual'
            if resolved and target.kind == k.CONSTRUCTOR:
                edge['kind'] = 'constructor'
            if not resolved:
                edge['dispatch'] = 'unresolved'
                gaps.append({'caller': owner, 'kind': 'unresolved_call', 'location': edge['location']})
            edges.append(edge)
        if cursor.kind in (k.CXX_NEW_EXPR, k.CXX_DELETE_EXPR):
            gaps.append({'caller': owner, 'kind': 'implicit_allocation_or_cleanup',
                         'location': location(cursor), 'expression': expression(cursor)})
        if cursor.kind == k.VAR_DECL and cursor.type.get_canonical().kind == ci.TypeKind.RECORD:
            gaps.append({'caller': owner, 'kind': 'automatic_object_lifetime',
                         'type': cursor.type.spelling, 'location': location(cursor)})
        for child in cursor.get_children():
            body(child, owner)

    def visit(cursor):
        if cursor.location.file and not project(cursor):
            return
        if cursor.kind in functions:
            owner = node(cursor)
            if owner and cursor.is_definition():
                for child in cursor.get_children():
                    body(child, owner)
            return
        for child in cursor.get_children():
            visit(child)

    visit(tu.cursor)
    files = {str(source.resolve()), *(str(Path(x.include.name).resolve()) for x in tu.get_includes())}
    return {'nodes': list(nodes.values()), 'edges': edges, 'gaps': gaps,
            'diagnostics': [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error],
            'inputs': {path: digest(path) for path in sorted(files)}}


def collect(root: Path, units: list[dict], *, fresh=False) -> dict:
    from homm3.analysis.access_facts import load_cindex
    from homm3.core.compiler_profile import Profiles
    from homm3.core.project import Project
    from clang import cindex as ci
    if not ci.Config.loaded:
        ci = load_cindex()
    profiles = Profiles(Project(root))
    profiles.mirror
    cache = root / 'build/mac/source-graph'
    cache.mkdir(parents=True, exist_ok=True)
    # Header additions and inactive branches can alter include resolution too.
    headers = [(str(p.relative_to(root)), digest(p)) for p in sorted((root / 'include').rglob('*')) if p.is_file()]
    library = Path(ci.conf.get_filename())
    shared = [SCHEMA, digest(__file__), str(root.resolve()), headers,
              str(library), library.stat().st_mtime_ns]
    records = []
    for number, unit in enumerate(units, 1):
        source = root / unit['source']
        args = [*profiles.for_source(source), '-U__clang__', '-D_MSC_VER=1200', '-DHOMM3_SOURCE_OWNERSHIP', '-ferror-limit=0']
        key = hashlib.sha256(json.dumps([shared, args]).encode()).hexdigest()
        path = cache / (hashlib.sha256(str(source).encode()).hexdigest() + '.json')
        saved = {}
        if not fresh and path.exists():
            try:
                saved = json.loads(path.read_text())
            except (ValueError, OSError):
                pass
        if not cache_valid(saved, key):
            try:
                saved = {'key': key, **scan(ci, source, args, root)}
            except ci.TranslationUnitLoadError as error:
                # Keep other units usable, but retry this failed parse next time.
                saved = {'key': key, 'nodes': [], 'edges': [], 'gaps': [],
                         'diagnostics': [f'cannot parse {source}: {error}'], 'inputs': {}}
            temporary = path.with_suffix('.tmp')
            temporary.write_text(json.dumps(saved))
            temporary.replace(path)
        records.append({'unit': source.stem, **saved})
        if number % 10 == 0 or number == len(units):
            print(f'[source graph] {number}/{len(units)} units indexed', file=sys.stderr)
    return merge(records)


def merge(records):
    nodes, edges, gaps, errors = {}, {}, [], {}
    views = defaultdict(dict)
    for record in records:
        unit = record['unit']
        errors[unit] = record['diagnostics']
        per_caller = defaultdict(list)
        for edge in record['edges']:
            per_caller[edge['caller']].append((edge['callee'], edge['location']['file'],
                edge['location']['offset'], edge['kind'], edge['dispatch'], edge['expression']))
        for item in record['nodes']:
            if item['definition']:
                signature = json.dumps(per_caller[item['id']], sort_keys=True)
                views[item['id']].setdefault(signature, []).append(unit)
            current = nodes.setdefault(item['id'], {**item, 'units': []})
            current['units'].append(unit)
            for field in ('mac', 'windows'):
                current[field] = sorted({tuple(x) for x in current[field] + item[field]})
            if item['definition']:
                current['definition'] = True
                current['location'] = item['location']
        for item in record['edges']:
            key = (item['caller'], item['callee'], item['location']['file'],
                   item['location']['offset'], item['kind'], item['dispatch'])
            current = edges.setdefault(key, {**item, 'units': []})
            current['units'].append(unit)
        gaps.extend({**g, 'unit': unit} for g in record['gaps'])
    for identity, node in nodes.items():
        node['body_views'] = list(views[identity].values())
    return {'schema': SCHEMA, 'units': list(errors), 'nodes': nodes,
            'edges': list(edges.values()), 'gaps': gaps, 'diagnostics': errors,
            'implicit_operations_complete': False}


def paths(graph, start, targets, *, max_depth=8):
    """Find source paths, without claiming Mac inlined any intervening helper."""
    outgoing = defaultdict(list)
    for edge in graph['edges']:
        if edge['callee'] and edge['dispatch'] == 'direct':
            outgoing[edge['caller']].append(edge)
    pending = deque([(start, [])])
    visited, found, truncated = {start}, {}, False
    while pending:
        caller, chain = pending.popleft()
        if len(graph['nodes'][caller].get('body_views', [])) > 1:
            # Do not invent a path by combining incompatible preprocessor views.
            truncated = True
            continue
        if len(chain) >= max_depth:
            truncated |= bool(outgoing[caller])
            continue
        for edge in outgoing[caller]:
            callee = edge['callee']
            trail = [*chain, edge]
            if callee in targets:
                found.setdefault(callee, trail)
            if callee not in visited:
                visited.add(callee)
                pending.append((callee, trail))
    return found, truncated
