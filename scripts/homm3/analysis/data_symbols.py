"""Narrow Clang/VC6 data spelling bridges, preserving complete ABI types.

These rules join a source declaration to candidate storage only. They establish
neither a retail address nor a retail extent. Unsupported back-reference changes
remain unbound; demangled display names are never an identity key.
"""
import re


CLANG_ANON = re.compile(r'@\?A0x[0-9a-fA-F]+@')
VC6_ANON = re.compile(r'@\?%([^@]+)@')
VC6_ORIGIN = re.compile(r'(.+\.(?:cpp|cxx|cc|c|hpp|hxx|h))([0-9]+)', re.IGNORECASE)


def anonymous_names(source, emitted, files):
    """Replace only observed anonymous scopes, keeping named scopes and types."""
    origins = []
    if not CLANG_ANON.search(source):
        return source, emitted, origins
    components = list(VC6_ANON.finditer(emitted))
    if not components:
        return source, emitted, origins
    for component in components:
        match = VC6_ORIGIN.fullmatch(component[1])
        if not match:
            return None
        path = match[1].replace('\\', '/').lower()
        choices = [f for f in files if path == f.lower() or path.endswith('/'+f.lower())]
        if len(choices) != 1:
            return None
        origins.append(dict(component=component[1], source_path=choices[0]))
    return (CLANG_ANON.sub('@{anonymous}@', source),
            VC6_ANON.sub('@{anonymous}@', emitted), origins)


def local_name(symbol, name, parent):
    """Discard only the compiler's local scope ordinal, not its enclosing ABI."""
    pattern = r'\?' + re.escape(name) + r'@\?(?:[0-9]|[A-P]+@)\?'
    match = re.match(pattern, symbol)
    if not match or not parent or not symbol[match.end():].startswith(parent+'@4'):
        return None
    return '?'+name+'@{local}?'+symbol[match.end():]


def spelling(fact, emitted, unit):
    """Return auditable evidence for one spelling pair, or no binding."""
    source = fact.get('symbol', '')
    if not source:
        return None
    rules, origins = [], []
    a, b = source, emitted
    # ABI bridges require a proven owning TU. Exact external spellings may be
    # observed at a header declaration without a definition in this snapshot.
    owned = fact.get('unit') == unit
    if fact.get('local') and b.startswith('_?') and owned:
        b = b[1:]
        rules.append('vc6-local-prefix')
    if a != b and owned:
        pair = anonymous_names(a, b, fact.get('anonymous_namespace_files', []))
        if pair is None:
            return None
        a, b, origins = pair
        if origins:
            rules.append('anonymous-namespace-origin')
        if fact.get('local'):
            parent = CLANG_ANON.sub('@{anonymous}@', fact.get('parent_symbol', '')) if origins else fact.get('parent_symbol', '')
            x, y = local_name(a, fact.get('name', ''), parent), local_name(b, fact.get('name', ''), parent)
            if x is not None and y is not None:
                a, b = x, y
                rules.append('local-static-discriminator')
        if (a != b and fact.get('reference_cell') and fact.get('const_array_reference') and
                '$$CB' in a and a.endswith('B') and b.endswith('A')):
            a = a[:-1]+'A'
            rules.append('const-array-reference-cell')
    if a != b:
        # VC6 can omit namespace/type encoding for an internal object. Both
        # named and anonymous namespaces exhibit this in the pinned compiler.
        # The caller must retain every compatible source entity/emission;
        # same short names in different namespaces are ambiguous, not aliases.
        if (owned and fact.get('linkage') == 'INTERNAL' and fact.get('local') is False and
                emitted == '_'+fact.get('name', '')):
            rules = ['vc6-internal-global']
        else:
            return None
    return dict(source_symbol=source, emitted_symbol=emitted, unit=unit,
                rules=rules or ['exact'], anonymous_origins=origins)
