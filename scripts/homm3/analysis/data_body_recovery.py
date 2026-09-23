"""Retain data declarations outside diagnosed, explicitly excluded bodies.

This is an unsaved analysis parse only. It never rewrites authored source or
compiler input. Exclusion requires ordinary, source-spelled bodies without
preprocessing activity; the remaining declarations must survive unchanged.
"""
from pathlib import Path
import hashlib


def same_file(location, path):
    return location.file is not None and Path(location.file.name).resolve() == path


def isolate(tu, path, diagnostics):
    import clang.cindex as cx
    path = path.resolve()
    raw = path.read_bytes()
    functions, macros = [], []
    kinds = (cx.CursorKind.FUNCTION_DECL, cx.CursorKind.CXX_METHOD,
             cx.CursorKind.CONSTRUCTOR, cx.CursorKind.DESTRUCTOR)

    def visit(node, ordinary=True):
        if node.location.file and not same_file(node.location, path):
            return
        children = list(node.get_children())
        if node.kind in (cx.CursorKind.FUNCTION_TEMPLATE, cx.CursorKind.CLASS_TEMPLATE,
                         cx.CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION):
            ordinary = False
        if node.kind == cx.CursorKind.MACRO_INSTANTIATION:
            macros.append(node.extent)
        if ordinary and node.kind in kinds:
            bodies = [c for c in children if c.kind == cx.CursorKind.COMPOUND_STMT]
            if len(bodies) == 1:
                body = bodies[0]
                a, b = body.extent.start, body.extent.end
                if (same_file(a, path) and same_file(b, path) and
                        0 <= a.offset < b.offset <= len(raw) and
                        raw[a.offset:a.offset+1] == b'{' and raw[b.offset-1:b.offset] == b'}'):
                    prefix = [t.spelling for t in node.get_tokens() if t.location.offset < a.offset]
                    # Deduced return types and constant evaluation can affect
                    # unrelated declarations through an otherwise absent body.
                    if (node.result_type.kind != cx.TypeKind.AUTO and
                            not {'auto', 'decltype', 'constexpr', 'consteval'} & set(prefix)):
                        functions.append((node, body))
        for child in children:
            visit(child, ordinary)
    visit(tu.cursor)
    chosen = {}
    for diagnostic in diagnostics:
        if diagnostic.severity >= cx.Diagnostic.Fatal or not same_file(diagnostic.location, path):
            return None, 'diagnostic outside an ordinary source body'
        owners = [(n, b) for n, b in functions if
                  b.extent.start.offset < diagnostic.location.offset < b.extent.end.offset-1]
        if not owners:
            return None, 'diagnostic outside an ordinary source body'
        node, body = max(owners, key=lambda p: p[1].extent.end.offset-p[1].extent.start.offset)
        a, b = body.extent.start.offset, body.extent.end.offset
        # Reject all preprocessing spellings/activity, including inactive
        # directives, digraph/trigraph directives and line-spliced directives.
        if any(token in raw[a:b] for token in (b'#', b'%:', b'??=', b'\\\n', b'\\\r', b'__pragma', b'_Pragma', b'__COUNTER__')) or any(
                same_file(m.start, path) and m.start.offset < b and a < m.end.offset for m in macros):
            return None, 'excluded body contains preprocessing activity'
        # A diagnostic range extending outside the body is not locally isolated.
        for extent in diagnostic.ranges:
            if not (same_file(extent.start, path) and same_file(extent.end, path) and
                    a < extent.start.offset <= extent.end.offset < b):
                return None, 'diagnostic range escapes its body'
        chosen[a] = dict(start=a, end=b, line=body.extent.start.line,
                         function=node.mangled_name, usr=node.get_usr(),
                         body_sha256=hashlib.sha256(raw[a:b]).hexdigest())
    regions = sorted(chosen.values(), key=lambda r: r['start'])
    if not regions or any(a['end'] > b['start'] for a, b in zip(regions, regions[1:])):
        return None, 'excluded bodies overlap or are unavailable'
    modified = bytearray(raw)
    for region in regions:
        for i in range(region['start']+1, region['end']-1):
            if modified[i] not in (10, 13):
                modified[i] = 32
    return dict(regions=regions, original_sha256=hashlib.sha256(raw).hexdigest(),
                analysis_sha256=hashlib.sha256(modified).hexdigest(), contents=bytes(modified)), ''


def declarations(tu, path, regions, signature):
    """Original and analysis parses must agree outside the excluded regions."""
    import clang.cindex as cx
    result = {}
    def visit(node):
        if same_file(node.location, path) and any(
                r['start'] < node.location.offset < r['end'] for r in regions):
            return
        if node.kind == cx.CursorKind.VAR_DECL and node.location.file:
            parent = node.semantic_parent
            local = parent.kind in (cx.CursorKind.FUNCTION_DECL, cx.CursorKind.CXX_METHOD,
                                    cx.CursorKind.CONSTRUCTOR, cx.CursorKind.DESTRUCTOR)
            if not local or node.storage_class.name in ('STATIC', 'EXTERN'):
                key = (str(Path(node.location.file.name).resolve()), node.location.offset, node.get_usr())
                result.setdefault(key, []).append((node.spelling, node.mangled_name, node.type.spelling,
                    node.storage_class.name, node.linkage.name, node.is_definition(),
                    parent.get_usr(), node.extent.start.offset, node.extent.end.offset, signature(node.type)))
        for child in node.get_children():
            visit(child)
    visit(tu.cursor)
    return result


def recover(index, tu, path, args, options, signature):
    import clang.cindex as cx
    diagnostics = [d for d in tu.diagnostics if d.severity >= cx.Diagnostic.Error]
    isolation, reason = isolate(tu, path, diagnostics)
    if isolation is None:
        return None, dict(status='rejected', reason=reason, regions=[])
    original = declarations(tu, path, isolation['regions'], signature)
    modified = isolation.pop('contents')
    evidence = dict(isolation, status='rejected', reason='', errors=[])
    try:
        contents = modified.decode('utf-8')
    except UnicodeDecodeError:
        return None, dict(evidence, reason='source encoding is not UTF-8')
    parsed = index.parse(str(path), args=args, options=options,
                         unsaved_files=[(str(path), contents)])
    errors = [str(d) for d in parsed.diagnostics if d.severity >= cx.Diagnostic.Error]
    if errors:
        return None, dict(evidence, reason='analysis reparse still has errors', errors=errors)
    observed = declarations(parsed, path, isolation['regions'], signature)
    if observed != original:
        return None, dict(evidence, reason='retained data declarations changed during reparse')
    return parsed, dict(evidence, status='isolated-bodies', retained_declarations=len(observed))
