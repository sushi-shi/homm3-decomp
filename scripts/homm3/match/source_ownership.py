"""Check canonical C++ definitions against Dreamcast CodeView source ownership.

Clang supplies declaration identities and physical definition locations only;
VC6 and the pinned retail image remain the code-generation authorities. In
particular, a retained COMDAT's link location does not own its source body.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
import csv
from dataclasses import asdict, dataclass
import hashlib
import json
from pathlib import Path
import re

from homm3.core import common, clang
from homm3 import manifest

ROOT = common.HOMM3_DIR


@dataclass(frozen=True)
class Definition:
    file: str
    line: int
    offset: int
    end: int
    name: str
    signature: str
    parameters: int
    member: bool
    inline: bool
    va: int | None
    mangled: str
    origin_file: str = ""
    origin_line: int = 0
    dc_offset: str = ""
    argument_types: tuple[str, ...] = ()
    const: bool = False
    class_offset: int | None = None


@dataclass(frozen=True)
class Origin:
    file: str
    name: str
    line: int
    parameters: int
    module: str
    offset: str
    argument_types: tuple[str, ...] | None = None
    const: bool = False
    generated: bool = False
    declaration_only: bool = False
    type_index: int = 0


def source_file(path: str) -> str:
    """Keep relative source subdirectories; ignore DOS drive/case differences."""
    name = re.sub('/+', '/', path.replace('\\', '/')).lower()
    if '/gamedcs/' in name:
        return name.split('/gamedcs/', 1)[1]
    return name


def read_dc(root: Path = ROOT, *, include_declarations: bool = False) -> list[Origin]:
    """Read procedure locations, optionally adding unlocated field-list identities.

    Location-only consumers such as the local-class cleanliness metric must
    not turn an un-emitted declaration into evidence for a source file.
    """
    from homm3.core import inputs
    from homm3.core.nb11_types import Types
    symbols = inputs.dreamcast_symbols()
    types = Types.from_symbols(symbols)
    generated = generated_members(types)
    origins = []
    with (root / 'evidence/dreamcast/functions.csv').open(newline='') as stream:
        rows = csv.DictReader(line for line in stream if not line.startswith('#'))
        for r in rows:
            proc = symbols.procedures.get(int(r['offset'], 16))
            function = types.get(proc.type_index) if proc else {}
            arguments = None
            const = False
            if function.get('kind') == 'function':
                arguments = tuple(types.declaration(t) for t in
                                  types.get(function['arguments']).get('types', []))
                this = types.get(function.get('this', 0))
                if this['kind'] == 'pointer':
                    const = 'const' in types.get(this['target']).get('qualifiers', [])
            origins.append(Origin(source_file(r['file']), r['name'], int(r['line'] or 0),
                                  int(r['params'] or 0), r['module'], r['offset'],
                                  arguments, const, bool(proc and
                                      (family_name(proc.name), proc.type_index) in generated)))
    if include_declarations:
        origins.extend(declaration_origins(types, origins))
    return origins


def member_records(types):
    """Enumerate named field-list methods, including LF_METHODLIST overloads."""
    for index in types.records:
        cls = types.get(index)
        if cls.get('kind') not in {'class', 'struct', 'union'} or cls.get('forward'):
            continue
        for field in types.fields(cls['fields']):
            methods = ([field] if field['kind'] == 'method' else
                       types.get(field['type']).get('entries', [])
                       if field['kind'] == 'overloads' else [])
            for method in methods:
                yield cls['name'] + '::' + field['name'], method


def declaration_origins(types, procedures: list[Origin]) -> list[Origin]:
    """Keep un-emitted member identities without inventing a body location.

    Field lists prove declarations and compgenx, but their member sequence is
    not a definition line table. In particular, the smart-pointer copy and
    pointer constructors are distinct even though only the latter have a
    procedure record. An absent body cannot inherit another overload's line.
    """
    seen = {(family_name(o.name), o.argument_types, o.const, o.generated)
            for o in procedures}
    result = []
    for name, method in member_records(types):
        function = types.get(method['type'])
        if function.get('kind') != 'function':
            continue
        arguments = tuple(types.declaration(t) for t in
                          types.get(function['arguments']).get('types', []))
        this = types.get(function.get('this', 0))
        const = (this.get('kind') == 'pointer' and
                 'const' in types.get(this['target']).get('qualifiers', []))
        generated = bool(method.get('attributes', 0) & 0x100)
        key = (family_name(name), arguments, const, generated)
        if key in seen:
            continue
        seen.add(key)
        result.append(Origin('', name, 0, len(arguments), '', '', arguments,
                             const, generated, True, method['type']))
    return result


def generated_members(types) -> set[tuple[str, int]]:
    """Read CV_fldattr_t.compgenx, never infer generation from line placement.

    Microsoft cvinfo.h defines bit 8 as a compiler-generated function that
    exists. Such a procedure's line can identify its emission/use site, not
    a written body. Keep both member name and function type: distinct methods
    with identical signatures can share an LF_MFUNCTION record.
    """
    result = set()
    for name, method in member_records(types):
        if method.get('attributes', 0) & 0x100:
            result.add((family_name(name), method['type']))
    return result


def family_name(name: str) -> str:
    """Template instances share one source definition, not one body count."""
    previous = None
    while previous != name:
        previous = name
        name = re.sub(r"(?<!operator)<[^<>]*>", "", name)
    return name


def type_identity(name: str) -> str:
    """Ignore elaborated-type syntax, preserving typedefs and qualifiers."""
    return re.sub(r'\s+', '', re.sub(r'\b(?:class|struct|enum|union)\s+', '', name))


def reference_stubs_only(raw: str) -> bool:
    """Unadmitted carcasses are evidence, but must contain no implementation."""
    from homm3.retail_labels import source
    from homm3.match.status import _definition_text
    masked = source.mask_lexical_noise(raw)
    residue = list(masked)
    for start, end, _args, _raw_args in source.macro_invocations(
            masked, source.MACRO_HEADS['DC_ONLY'][0], raw):
        if end is None:
            return False
        body = _definition_text(raw, masked, end + 1)
        if body is None:
            return False
        begin = raw.index(body, end + 1)
        finish = begin + len(body)
        declaration = masked[begin:finish]
        if declaration[declaration.index('{') + 1:declaration.rindex('}')].strip():
            return False
        residue[start:finish] = ' ' * (finish - start)
    # Only includes and the conventional inactive-carcass wrapper are allowed;
    # a macro definition could itself hide a new implementation.
    remaining = re.sub(r'^\s*#\s*(?:include\b[^\n]*|if\s+0\s*|endif\s*)$',
                       '', ''.join(residue), flags=re.M)
    return not remaining.strip()


def origin_hint(raw: str, start: int) -> tuple[str, int, str]:
    # Only the attached comment/declarator prefix is eligible. Never carry an
    # origin across another definition (the old link-order parser did that).
    line_start = raw.rfind('\n', 0, start) + 1
    lines = raw[:line_start].splitlines()
    prefix = []
    for line in reversed(lines):
        text = line.strip()
        if text and not text.startswith(('//', '#', 'VA(', 'DC_ONLY(')):
            break
        prefix.append(line)
    prefix.reverse()
    origin_file, origin_line, dc_offset = '', 0, ''
    for line in prefix:
        renamed = re.fullmatch(
            r"//\s+Original:\s+[^;]+;\s+([^;,]+\.(?:cpp|h)):(\d+),\s+dc\s+(0x[0-9a-fA-F]+)\.?(?:\s.*)?",
            line.strip())
        if renamed:
            origin_file = source_file(renamed.group(1))
            origin_line = int(renamed.group(2))
            dc_offset = hex(int(renamed.group(3), 16))
        m = re.match(r"//\s+([A-Za-z]:\\.+?\.(?:cpp|h)):(\d+)(?:[.,\s]|$)", line.strip())
        if m:
            origin_file, origin_line = source_file(m.group(1)), int(m.group(2))
        offsets = re.findall(r"\bdc\s+(0x[0-9a-fA-F]+)\b", line)
        if offsets and line.lstrip().startswith('VA('):
            dc_offset = hex(int(offsets[-1], 16))
    return origin_file, origin_line, dc_offset


def _qualified(cursor, kinds) -> str:
    parts = [cursor.spelling]
    parent = cursor.semantic_parent
    while parent is not None and parent.kind != kinds.TRANSLATION_UNIT:
        if parent.spelling:
            parts.insert(0, parent.spelling)
        parent = parent.semantic_parent
    return '::'.join(parts)


def declaration_name(cursor) -> str:
    """Use the source destructor variant, not libclang's vbase closure name."""
    from clang import cindex
    if cursor.kind != cindex.CursorKind.DESTRUCTOR:
        return cursor.mangled_name
    import ctypes
    # Python's binding exposes only clang_Cursor_getMangling, which returns
    # ??_D for an MSVC destructor. The C API supplies the actual ??1 variant.
    # Raw strings avoid _CXString.__del__ double-freeing StringSet storage.
    class RawString(ctypes.Structure):
        _fields_ = [('data', ctypes.c_void_p), ('private_flags', ctypes.c_uint)]
    class StringSet(ctypes.Structure):
        _fields_ = [('Strings', ctypes.POINTER(RawString)), ('Count', ctypes.c_uint)]
    get = ctypes.CFUNCTYPE(ctypes.POINTER(StringSet), cindex.Cursor)(
        ('clang_Cursor_getCXXManglings', cindex.conf.lib))
    string = ctypes.CFUNCTYPE(ctypes.c_char_p, RawString)(
        ('clang_getCString', cindex.conf.lib))
    dispose = ctypes.CFUNCTYPE(None, ctypes.POINTER(StringSet))(
        ('clang_disposeStringSet', cindex.conf.lib))
    names = get(cursor)
    try:
        variants = [string(names.contents.Strings[i]).decode()
                    for i in range(names.contents.Count)] if names else []
        ordinary = [name for name in variants if name.startswith('??1')]
        return ordinary[0] if len(ordinary) == 1 else cursor.mangled_name
    finally:
        if names:
            dispose(names)


def scan_unit(unit: dict, root: Path = ROOT) -> tuple[list[Definition], list[str], list[str]]:
    """Fail visibly on parse errors; never turn an unreadable TU into no bodies."""
    from clang import cindex
    import ctypes
    k = cindex.CursorKind
    is_inlined = cindex.conf.lib.clang_Cursor_isFunctionInlined
    is_inlined.argtypes = [cindex.Cursor]
    is_inlined.restype = ctypes.c_uint
    function_kinds = {k.FUNCTION_DECL, k.CXX_METHOD, k.CONSTRUCTOR,
                      k.DESTRUCTOR, k.FUNCTION_TEMPLATE, k.CONVERSION_FUNCTION}
    args = ['--driver-mode=cl', '/TP', *clang.FLAGS, '-imsvc', str(clang.mirror()),
            '/I' + str(root / 'include'), '/I' + str(root / 'vendor/zlib-1.1.3')]
    tu = cindex.Index.create().parse(
        str(root / unit['source']), args=args,
        options=cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
    from homm3.vc6 import _source
    texts = {}
    raw_texts = {}
    byte_texts = {}
    errors = [f"PARSE {unit['source']}: {d}" for d in tu.diagnostics
              if d.severity >= cindex.Diagnostic.Error]
    definitions = []
    reached = set()

    def visit(cursor):
        loc = cursor.location
        relative = None
        if loc.file:
            try:
                relative = Path(loc.file.name).relative_to(root).as_posix()
            except ValueError:
                return
            if not relative.startswith(('src/', 'include/')):
                return
            reached.add(relative)
        if cursor.kind in function_kinds:
            if not relative:
                return
            # SKIP_FUNCTION_BODIES avoids interpreting VC6's legacy local
            # lookup rules. The AST retains the active declarator and its
            # exact end; inspect only the following balanced body extent.
            if relative not in texts:
                raw_texts[relative] = (root / relative).read_text()
                texts[relative] = _source._mask_lex(raw_texts[relative])
                byte_texts[relative] = raw_texts[relative].encode('utf-8')
            def char_offset(offset):
                encoded = byte_texts[relative]
                if len(encoded) == len(raw_texts[relative]):
                    return offset
                # Clang reports byte offsets; all balanced lexical helpers
                # below use Python character offsets. Comments may be UTF-8.
                return len(encoded[:offset].decode('utf-8'))
            masked = texts[relative]
            opening = _source._skip_ws(masked, char_offset(cursor.extent.end.offset))
            if opening < len(masked) and masked[opening] == ':':
                opening = _source._skip_init_list(masked, opening)
            if opening is None or opening >= len(masked) or masked[opening] != '{':
                return
            closing = _source._match_paren(masked, opening, '{', '}')
            if closing is None:
                errors.append(f'PARSE {relative}:{loc.line}: unbalanced definition body')
                return
            attrs = [c.spelling for c in cursor.get_children()
                     if c.kind == k.ANNOTATE_ATTR]
            vas = [int(m.group(1), 16) for a in attrs
                   if (m := re.fullmatch(r'va:(0[xX][0-9a-fA-F]+) size:.*', a))]
            member = cursor.kind in {k.CXX_METHOD, k.CONSTRUCTOR, k.DESTRUCTOR,
                                     k.CONVERSION_FUNCTION}
            if cursor.kind == k.CXX_METHOD and cursor.is_static_method():
                member = False
            parent = cursor.lexical_parent
            class_offset = None
            while parent.kind in {k.CLASS_DECL, k.STRUCT_DECL, k.UNION_DECL, k.CLASS_TEMPLATE}:
                class_offset = char_offset(parent.extent.start.offset)
                parent = parent.lexical_parent
            definitions.append(Definition(
                relative, loc.line, char_offset(cursor.extent.start.offset),
                closing + 1, _qualified(cursor, k),
                re.sub(r'\s+noexcept\b', '', cursor.type.spelling),
                sum(c.kind == k.PARM_DECL for c in cursor.get_children()),
                member, bool(is_inlined(cursor)),
                vas[0] if len(vas) == 1 else None, declaration_name(cursor),
                *origin_hint(raw_texts[relative], char_offset(cursor.location.offset)),
                tuple(c.type.spelling for c in cursor.get_children() if c.kind == k.PARM_DECL),
                cursor.is_const_method() if cursor.kind in {k.CXX_METHOD, k.CONVERSION_FUNCTION} else False,
                class_offset))
            return  # locals/calls are not definitions in another source file
        for child in cursor.get_children():
            visit(child)

    visit(tu.cursor)
    return definitions, errors, sorted(reached)


def collect(root: Path = ROOT, jobs: int = 4, fresh: bool = False):
    units = [u for u in manifest.units(root / 'config/units.toml')
             if u['source'].startswith('src/')]
    # Conservative cache key: changing any owned source/header invalidates all
    # TUs, including consumers that do not emit an out-of-line inline copy.
    digest = hashlib.sha256(Path(__file__).read_bytes())
    for dependency in ('scripts/homm3/core/clang.py', 'scripts/homm3/vc6/_source.py',
                       'scripts/homm3/manifest.py'):
        digest.update((root / dependency).read_bytes())
    for base in ('src', 'include', 'config'):
        for path in sorted((root / base).rglob('*')):
            if path.is_file() and path.suffix.lower() in {'.h', '.hpp', '.inl', '.c', '.cpp', '.cxx', '.toml'}:
                digest.update(str(path.relative_to(root)).encode())
                digest.update(path.read_bytes())
    cache = root / 'build/source-ownership/definitions.json'
    key = digest.hexdigest()
    if not fresh and cache.is_file():
        saved = json.loads(cache.read_text())
        if saved['key'] == key:
            return ([Definition(**r) for r in saved['definitions']],
                    saved['errors'], saved['reached'])
    clang.mirror()  # construct shared mirror before starting workers
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(lambda u: scan_unit(u, root), units))
        reached = {p for _, _, paths in results for p in paths}
        orphan_headers = [dict(source=p.relative_to(root).as_posix())
                          for p in sorted((root / 'include').rglob('*'))
                          if p.suffix.lower() in {'.h', '.hpp', '.inl'}
                          and p.relative_to(root).as_posix() not in reached]
        results.extend(pool.map(lambda u: scan_unit(u, root), orphan_headers))
    unique = {}
    errors = []
    admitted = {u['source'] for u in units}
    for path in sorted((root / 'src').rglob('*')):
        if path.suffix.lower() not in {'.c', '.cpp', '.cxx'}:
            continue
        relative = path.relative_to(root).as_posix()
        if relative not in admitted and not reference_stubs_only(path.read_text()):
            errors.append(f'COVERAGE {relative}: implementation outside config/units.toml')
    reached = set()
    for definitions, failures, paths in results:
        errors.extend(failures)
        reached.update(paths)
        for d in definitions:
            unique[(d.file, d.offset, d.name, d.signature)] = d
    definitions = sorted(unique.values(), key=lambda d: (d.file, d.offset, d.signature))
    cache.parent.mkdir(parents=True, exist_ok=True)
    cache.write_text(json.dumps(dict(key=key, definitions=[asdict(d) for d in definitions],
                                    errors=errors, reached=sorted(reached)), indent=2) + '\n')
    return definitions, errors, sorted(reached)


def read_filter(path: Path, fields: tuple[str, ...]):
    if not path.is_file():
        return {}, [f'FILTER missing {path.name}']
    entries = {}
    errors = []
    with path.open(newline='') as stream:
        rows = csv.DictReader((l for l in stream if not l.startswith('#')), delimiter='\t')
        if rows.fieldnames != [*fields, 'reason']:
            return {}, [f'FILTER {path.name}: expected columns {(*fields, "reason")}']
        for row in rows:
            key = tuple(row.get(f, '') for f in fields)
            if key in entries or not all(key) or not row.get('reason', '').strip():
                errors.append(f'FILTER {path.name}: duplicate/incomplete entry {key}')
            entries[key] = row.get('reason', '')
    return entries, errors


def compare(definitions: list[Definition], origins: list[Origin], dc_only: dict,
            win_only: dict) -> tuple[list[str], dict]:
    by_name = defaultdict(list)
    dc_keys = set()
    for o in origins:
        key = (o.file, o.name, str(o.line))
        if not o.declaration_only:
            dc_keys.add(key)
        if key not in dc_only:
            by_name[family_name(o.name)].append(o)
    errors = [f'FILTER stale dc_only.tsv entry {key}' for key in dc_only if key not in dc_keys]
    used_win = set()
    matches = []
    bindings = {}
    counts = Counter()
    for d in definitions:
        where = f'{d.file}:{d.line} {d.name}'
        key = (d.file, d.name, d.signature)
        candidates = by_name.get(family_name(d.name), [])
        explicit_identity = False
        if d.dc_offset:
            bridged = [o for o in origins if o.offset == d.dc_offset
                       and (o.file, o.name, str(o.line)) not in dc_only]
            if bridged:
                candidates = bridged
                explicit_identity = True
        if d.origin_file and d.origin_line:
            narrowed = [o for o in candidates if o.file == d.origin_file
                        and o.line == d.origin_line]
            if narrowed:
                candidates = narrowed
                explicit_identity = True
        # Formal CodeView types exclude hidden ABI arguments and survive when
        # optimized debug variable records omit unused source parameters.
        narrowed = [o for o in candidates if o.argument_types is not None
                    and len(o.argument_types) == d.parameters and o.const == d.const]
        signature_mismatch = []
        if narrowed:
            candidates = narrowed
        elif not explicit_identity:
            signature_mismatch = [o for o in candidates if o.argument_types is not None]
            candidates = [o for o in candidates if o.argument_types is None]
        narrowed = [o for o in candidates if o.argument_types is not None
                    and tuple(type_identity(t) for t in o.argument_types)
                    == tuple(type_identity(t) for t in d.argument_types)]
        if not narrowed:
            # Template definitions have T where CV records a concrete class
            # instantiation. Keep the containing type and its qualifiers:
            # const SmartPtr<T>& can identify the copy constructor, whereas
            # T* cannot borrow that declaration's identity.
            narrowed = [o for o in candidates if o.argument_types is not None
                        and tuple(type_identity(family_name(t)) for t in o.argument_types)
                        == tuple(type_identity(family_name(t)) for t in d.argument_types)]
        if narrowed:
            candidates = narrowed
        elif any(not o.declaration_only for o in candidates):
            # Unlocated declarations participate in overload selection when
            # their formal types match. Mere name/arity agreement must not
            # give them precedence over the existing procedure candidates.
            candidates = [o for o in candidates if not o.declaration_only]
        written = [o for o in candidates if not o.generated]
        if key in win_only:
            used_win.add(key)
            if written:
                errors.append(f'FILTER {where}: Windows-only exemption hides a CodeView counterpart')
            else:
                counts['win_only'] += 1
            continue
        if signature_mismatch and not candidates:
            expected = sorted({(f'type {o.type_index:#x} (declaration only)'
                                if o.declaration_only else f'{o.file}:{o.line}')
                               + f' ({", ".join(o.argument_types)})'
                               + (' const' if o.const else '')
                               for o in signature_mismatch})
            errors.append(f'SIGNATURE {where} [{d.signature}]: no matching CodeView '
                          'formal arity/constness; review overload identity or the '
                          'platform signature change against ' + '; '.join(expected))
            counts['signature'] += 1
            continue
        if not written:
            if candidates:
                errors.append(f'GENERATED {where} [{d.signature}]: CodeView counterpart '
                              'is compiler-generated; restore the implicit member or '
                              'review the Windows-specific definition')
                counts['generated'] += 1
            else:
                errors.append(f'WIN_ONLY {where} [{d.signature}]: no CodeView counterpart or reviewed exemption')
                counts['unknown'] += 1
            continue
        candidates = written
        if all(o.declaration_only for o in candidates):
            expected = sorted({f'{o.name} ({", ".join(o.argument_types or ())})'
                               + (' const' if o.const else '')
                               + f' [type {o.type_index:#x}]' for o in candidates})
            errors.append(f'UNLOCATED {where} [{d.signature}]: CodeView declares '
                          + '; '.join(expected) + '; no procedure source location; '
                          'recover body ownership/order without borrowing another overload\'s line')
            counts['unlocated'] += 1
            continue
        candidates = [o for o in candidates if not o.declaration_only]
        # One written DC function cannot authorize multiple physical Windows
        # definitions. In particular, a same-arity adapter overload must not
        # borrow the canonical helper's owner merely because type narrowing
        # fell back to its name. Ignore repeated emissions of the same DC
        # source definition, but preserve real overloads and instantiations.
        identities = {(o.file, o.name, o.line, o.argument_types, o.const)
                      for o in candidates if o.line}
        if len(identities) == 1:
            identity = next(iter(identities))
            prior = bindings.get(identity)
            if prior and (prior.file, prior.offset) != (d.file, d.offset):
                errors.append(f'DUPLICATE {where} [{d.signature}]: CodeView '
                              f'{identity[0]}:{identity[2]} {identity[1]} already '
                              f'binds {prior.file}:{prior.line} {prior.name} '
                              f'[{prior.signature}]; restore one canonical body '
                              'or prove a distinct platform overload')
                counts['duplicate'] += 1
            else:
                bindings[identity] = d
        locations = {(o.file, o.line) for o in candidates}
        files = {f for f, _ in locations}
        actual = d.file.split('/', 1)[1].lower()
        if actual not in files:
            errors.append(f'OWNER {where}: CodeView defines in {", ".join(sorted(files))}')
            counts['owner'] += 1
        else:
            lines = {line for f, line in locations if f == actual and line}
            if len(lines) == 1:
                matches.append((d, actual, next(iter(lines))))
            elif len(lines) > 1:
                errors.append(f'AMBIGUOUS {where}: CodeView source lines {sorted(lines)} need overload identity')
            counts['same_file'] += 1
    for key in win_only.keys() - used_win:
        errors.append(f'FILTER stale win_only.tsv entry {key}')
    previous = {}
    for d, file, dc_line in matches:
        # Ordinary retained .cpp bodies follow retail RVA order, checked by
        # verify_va_claims. The older DC build can order those bodies
        # differently (townObject's constructor / HandleGiftMsg, for example).
        # Header bodies, explicit inlines and unclaimed helpers instead use
        # the source-line stream; an ordinary retail claim cannot advance it.
        if d.file.startswith('src/') and d.va is not None and not d.inline:
            continue
        key = (d.file, file)
        prior = previous.get(key)
        if prior and dc_line < prior[1]:
            errors.append(f'ORDER {d.file}:{d.line} {d.name} (DC {dc_line}) follows {prior[0].name} (DC {prior[1]})')
            counts['order'] += 1
        previous[key] = (d, dc_line)
    return errors, dict(counts)


def audit(root: Path = ROOT, jobs: int = 4, fresh: bool = False):
    from homm3.retail_labels.fragments import all_claims
    definitions, errors, reached = collect(root, jobs, fresh)
    dc_only, failures = read_filter(root / 'config/dc_only.tsv', ('file', 'function', 'line'))
    errors.extend(failures)
    win_only, failures = read_filter(root / 'config/win_only.tsv', ('file', 'function', 'signature'))
    errors.extend(failures)
    violations, counts = compare(definitions, read_dc(root, include_declarations=True),
                                 dc_only, win_only)
    errors.extend(violations)
    claims = all_claims()
    errors.extend(header_claim_ownership(definitions, claims))
    errors.extend(claim_identity(definitions, claims))
    errors.extend(compgen_identity(claims))
    return dict(definitions=len(definitions), counts=counts, violations=errors, reached=reached)


def header_claim_ownership(definitions: list[Definition], claims) -> list[str]:
    """A matching inactive .cpp stub cannot substitute for the header VA."""
    by_name = defaultdict(set)
    for claim in claims:
        if claim.kind == 'func' and claim.channel.startswith('src-VA'):
            by_name[claim.name].add(claim.rva + common.IMAGE_BASE)
    errors = []
    for d in definitions:
        if not d.file.startswith('include/') or not d.mangled:
            continue
        addresses = by_name.get(d.mangled, set())
        if addresses and d.va not in addresses:
            errors.append(f'CLAIM_OWNER {d.file}:{d.line} {d.name}: canonical header body '
                          f'needs its VA annotation ({", ".join(hex(a) for a in sorted(addresses))})')
    return errors


def claim_identity(definitions: list[Definition], claims) -> list[str]:
    """Retaining an RVA under a raw placeholder is not retaining its identity."""
    by_address = defaultdict(set)
    for claim in claims:
        if claim.kind == 'func':
            by_address[claim.rva + common.IMAGE_BASE].add(claim.name)
    return [f'CLAIM_IDENTITY {d.file}:{d.line} {d.name}: {hex(d.va)} must name '
            f'{d.mangled!r}, extracted {sorted(by_address[d.va])!r}'
            for d in definitions if d.va is not None and d.mangled
            and d.mangled not in by_address[d.va]]


def compgen_identity(claims) -> list[str]:
    """Generated-member enrollment must bind an actual VC6-emitted symbol.

    Anonymous initialization thunks acquire synthetic names deliberately.
    Other VA_COMPGEN kinds join a pre-existing public; retaining their raw
    enrollment name means no public was paired, even if the RVA survived.
    """
    from homm3.retail_labels.source import ANONYMOUS_COMPGEN_KINDS
    return [f'CLAIM_COMPGEN {c.unit}: {hex(c.rva + common.IMAGE_BASE)} '
            f'{c.meta["ckind"]} {c.meta.get("owner", "")}: '
            'no VC6 function paired with the source enrollment'
            for c in claims if c.kind == 'func'
            and c.channel.startswith('src-VA') and c.meta.get('ckind')
            and c.meta['ckind'] not in ANONYMOUS_COMPGEN_KINDS
            and c.name.startswith('__h3cg$')]


def run_gate() -> list[str]:
    result = audit()
    print(f"[build] source-ownership: {result['definitions']} canonical definitions; "
          f"{len(result['violations'])} violations")
    return result['violations']


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--json', action='store_true')
    parser.add_argument('--fresh', action='store_true')
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args(argv)
    result = audit(jobs=args.jobs, fresh=args.fresh)
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(f"{result['definitions']} definitions: {result['counts']}")
        print('\n'.join(result['violations']))
    return bool(result['violations'])


if __name__ == '__main__':
    raise SystemExit(main())
