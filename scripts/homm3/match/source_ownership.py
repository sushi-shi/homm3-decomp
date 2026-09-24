"""Scan canonical C++ definitions and check their retail claim ownership.

Clang supplies declaration identities and physical definition locations only;
VC6 and the pinned retail image remain the code-generation authorities. In
particular, a retained COMDAT's link location does not own its source body.
"""
from __future__ import annotations

import argparse
from bisect import bisect_left
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
from dataclasses import asdict, dataclass, replace
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile
from typing import TypedDict

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
    argument_types: tuple[str, ...] = ()
    const: bool = False
    class_offset: int | None = None
    variadic: bool = False
    instance: str = ""
    # Extra retained specializations share this one physical source body.
    additional_instances: tuple[tuple[int, str, str], ...] = ()
    return_type: str = ""
    template: bool = False
    internal: bool = False


class LineIndex:
    """One index per scanned file, shared by all annotation lookups."""
    def __init__(self, raw: str):
        self.raw = raw
        self.lines = raw.splitlines(keepends=True)
        self.starts = []
        offset = 0
        for line in self.lines:
            self.starts.append(offset)
            offset += len(line)
        self.prefixes = {}

    def preceding(self, start):
        beginning = self.raw.rfind('\n', 0, start) + 1
        for index in range(bisect_left(self.starts, beginning) - 1, -1, -1):
            yield self.lines[index]


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


def instance_annotations(raw: str, start: int, declaration: int, index: LineIndex | None = None):
    """Pair each selector comment with its following VA on this declaration."""
    from homm3.retail_labels import source
    beginning = raw.rfind('\n', 0, start) + 1
    index = index or LineIndex(raw)
    for line in index.preceding(start):
        stripped = line.strip()
        if stripped and not stripped.startswith(('//', 'VA(')):
            break
        if stripped.startswith('VA(') and '{' in source.mask_lexical_noise(line):
            break  # a preceding one-line definition is not this claim's prefix
        beginning -= len(line)
    attached = raw[beginning:declaration]
    hints = [(m.start(), 'hint', m.group(1)) for m in re.finditer(
        r'(?m)^[ \t]*//\s*VA instance:[ \t]*(.*?)[ \t]*$', attached)]
    if not hints:
        return [], False
    masked = source.mask_lexical_noise(attached)
    annotations = [(pos, 'va', int(args[0].strip(), 16))
                   for pos, end, args, _ in source.macro_invocations(
                       masked, source.MACRO_HEADS['VA'][0], attached)
                   if end is not None and len(args) == 2]
    pending, paired = [], []
    invalid = False
    for _pos, kind, value in sorted(hints + annotations):
        if kind == 'hint':
            pending.append(value)
        else:
            if len(pending) != 1 or not pending[0]:
                invalid = True
            else:
                paired.append((value, pending[0]))
            pending = []
    invalid |= bool(pending)
    invalid |= len({va for va, _ in paired}) != len(paired)
    invalid |= len({selector for _, selector in paired}) != len(paired)
    return paired, invalid


def claim_definitions(definitions):
    """Expand retained claims without counting template bodies more than once."""
    for d in definitions:
        yield d
        for va, selector, mangled in getattr(d, 'additional_instances', ()):
            yield replace(d, va=va, instance=selector, mangled=mangled,
                          additional_instances=())


def resolve_instances(definitions, requests, unit, root, args):
    """Resolve selected member names in an unsaved AST-only probe.

    The probe is never compiled or written to project source. Its reference
    must point back to the annotated generic declaration's exact physical
    token, so a sibling member or explicit specialization cannot steal a VA.
    """
    from clang import cindex
    kinds = cindex.CursorKind
    probes = []
    expected = {}
    constructor_parameters = {}
    errors = []
    for index, token_offset in requests:
        d = definitions[index]
        name = f'__homm3_va_instance_{index}'
        constructor = re.fullmatch(
            r'([A-Za-z_][\w:<>, *&]*<[^()]+>)::([A-Za-z_]\w*)\(([^()]*)\)',
            d.instance)
        function = re.fullmatch(
            r'((?:[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*|operator<)\s*'
            r'<[\w:<>, *&]+>)', d.instance)
        if constructor:
            owner, member, arguments = constructor.groups()
            if member != owner.split('<', 1)[0].rpartition('::')[2]:
                errors.append(f'INSTANCE {d.file}:{d.line} {d.name}: invalid constructor selector {d.instance!r}')
                continue
            parameters, start, depth = [], 0, 0
            for position, character in enumerate(arguments + ','):
                depth += (character == '<') - (character == '>')
                if character == ',' and depth == 0:
                    parameters.append(arguments[start:position].strip())
                    start = position + 1
            if not arguments.strip():
                parameters = []
            if depth or any(not re.fullmatch(r'[A-Za-z_][\w:<>, *&]*', p)
                            for p in parameters):
                errors.append(f'INSTANCE {d.file}:{d.line} {d.name}: invalid constructor parameter types {d.instance!r}')
                continue
            # A constructor has no address. Only the unsaved AST sees these
            # typed lvalues; no probe is compiled or added to game source.
            aliases = [f'{name}_parameter_{i}' for i in range(len(parameters))]
            probes.extend(f'typedef {parameter} {alias};'
                          for parameter, alias in zip(parameters, aliases))
            constructor_parameters[name] = aliases
            values = ', '.join('*static_cast<' + p.rstrip('& ').strip() + '*>(0)'
                               for p in parameters)
            probes.append(f'auto {name} = new {owner}({values});')
            expected[name] = (index, token_offset)
            continue
        if function:
            probes.append(f'auto {name} = &{d.instance};')
            expected[name] = (index, token_offset)
            continue
        owner, separator, member = d.instance.rpartition('::')
        if (not separator or '<' not in owner
                or not re.fullmatch(r'[A-Za-z_][\w:<>, *&]*', owner)
                or not re.fullmatch(r'(?:~?[A-Za-z_]\w*|operator\*|operator\(\))', member)):
            errors.append(f'INSTANCE {d.file}:{d.line} {d.name}: invalid class-member selector {d.instance!r}')
            continue
        if member.startswith('~'):
            # A destructor cannot have its address taken. A never-executed
            # initializer expression supplies the same declaration identity.
            alias = name + '_type'
            probes.append(f'typedef {owner} {alias};\n'
                          f'int {name} = ((({owner}*)0)->~{alias}(), 0);')
        else:
            probes.append(f'auto {name} = &{d.instance};')
        expected[name] = (index, token_offset)
    if not probes:
        return definitions, errors
    path = root / unit['source']
    original = path.read_text()
    probe = original + '\n' + '\n'.join(probes) + '\n'
    tu = cindex.Index.create().parse(
        str(path), args=[*args, '-fno-access-control'],
        unsaved_files=[(str(path), probe)],
        options=cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
    failures = [str(d) for d in tu.diagnostics
                if d.severity >= cindex.Diagnostic.Error]
    if failures:
        return definitions, errors + [f'INSTANCE {unit["source"]}: {d}' for d in failures]
    root_cursor = tu.cursor
    if root_cursor is None:
        return definitions, errors + [f'INSTANCE {unit["source"]}: Clang returned no translation-unit cursor']
    callable_kinds = {kinds.CXX_METHOD, kinds.CONSTRUCTOR,
                      kinds.DESTRUCTOR, kinds.FUNCTION_DECL}
    parameter_types = {
        cursor.spelling: cursor.underlying_typedef_type.get_canonical()
        for cursor in root_cursor.get_children() if cursor.kind == kinds.TYPEDEF_DECL
    }
    def references(cursor):
        found = {}
        ref = cursor.referenced
        if ref and ref.kind in callable_kinds:
            found[(ref.location.file.name if ref.location.file else '',
                   ref.location.offset, declaration_name(ref))] = ref
        for child in cursor.get_children():
            found.update(references(child))
        return found
    seen = set()
    for cursor in root_cursor.get_children():
        if cursor.kind != kinds.VAR_DECL or cursor.spelling not in expected:
            continue
        seen.add(cursor.spelling)
        index, offset = expected[cursor.spelling]
        d = definitions[index]
        targets = references(cursor)
        if len(targets) != 1:
            errors.append(f'INSTANCE {d.file}:{d.line} {d.name}: selector {d.instance!r} does not identify one member')
            continue
        (file, target_offset, mangled), ref = next(iter(targets.items()))
        if cursor.spelling in constructor_parameters:
            requested = [parameter_types[alias]
                         for alias in constructor_parameters[cursor.spelling]]
            actual = [argument.type.get_canonical() for argument in ref.get_arguments()]
            if len(requested) != len(actual) or any(a != b for a, b in zip(requested, actual)):
                errors.append(f'INSTANCE {d.file}:{d.line} {d.name}: constructor parameter types do not match selector {d.instance!r}')
                continue
        if file != str(root / d.file) or target_offset != offset or not mangled:
            errors.append(f'INSTANCE {d.file}:{d.line} {d.name}: selector {d.instance!r} does not name the annotated definition')
            continue
        # Check the destructor spelling too: the alias expression above
        # identifies the owner's destructor, not an arbitrary trailing name.
        if d.instance.rpartition('::')[2].startswith('~'):
            selected = d.instance.rpartition('::')[2]
            if selected != ref.spelling.split('<', 1)[0]:
                errors.append(f'INSTANCE {d.file}:{d.line} {d.name}: invalid destructor selector {d.instance!r}')
                continue
        definitions[index] = replace(d, mangled=mangled)
    for name in expected.keys() - seen:
        d = definitions[expected[name][0]]
        errors.append(f'INSTANCE {d.file}:{d.line} {d.name}: selector {d.instance!r} was not resolved')
    return definitions, errors


def scan_unit(unit: dict, root: Path = ROOT, *, profiles=None) -> tuple[list[Definition], list[str], list[str]]:
    """Fail visibly on parse errors; never turn an unreadable TU into no bodies."""
    from clang import cindex
    import ctypes
    k = cindex.CursorKind
    is_inlined = cindex.conf.lib.clang_Cursor_isFunctionInlined
    is_inlined.argtypes = [cindex.Cursor]
    is_inlined.restype = ctypes.c_uint
    function_kinds = {k.FUNCTION_DECL, k.CXX_METHOD, k.CONSTRUCTOR,
                      k.DESTRUCTOR, k.FUNCTION_TEMPLATE, k.CONVERSION_FUNCTION}
    # Inventory the matching compiler's project branches, including written
    # definitions hidden from the editor behind !defined(__clang__). Keep
    # annotations available through va.h without selecting editor-only code.
    from homm3.core.compiler_profile import Profiles
    from homm3.core.project import Project
    profiles = profiles or Profiles(Project(root))
    args = [*profiles.for_source(root / unit['source']), '-U__clang__',
            '-D_MSC_VER=' + clang.MSC_VER, '-DHOMM3_SOURCE_OWNERSHIP']
    tu = cindex.Index.create().parse(
        str(root / unit['source']), args=args,
        options=cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
    from homm3.vc6 import _source
    texts = {}
    raw_texts = {}
    byte_texts = {}
    line_indexes = {}
    relative_paths = {}

    def relative_path(name):
        if name not in relative_paths:
            try:
                relative_paths[name] = Path(name).relative_to(root).as_posix()
            except ValueError:
                relative_paths[name] = None
        return relative_paths[name]
    errors = [f"PARSE {unit['source']}: {d}" for d in tu.diagnostics
              if d.severity >= cindex.Diagnostic.Error]
    definitions = []
    instance_requests = []
    instance_groups = []
    reached = set()
    # Include files with no declarations too: a macro-only include can change
    # ownership facts in its consumer. This also covers included .cpp files.
    for inclusion in tu.get_includes():
        relative = relative_path(inclusion.include.name)
        if relative is not None:
            reached.add(relative)

    def visit(cursor):
        loc = cursor.location
        relative = None
        if loc.file:
            relative = relative_path(loc.file.name)
            if relative is None or not relative.startswith(('src/', 'include/')):
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
                line_indexes[relative] = LineIndex(raw_texts[relative])
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
            template = cursor.kind == k.FUNCTION_TEMPLATE
            while parent.kind in {k.CLASS_DECL, k.STRUCT_DECL, k.UNION_DECL, k.CLASS_TEMPLATE}:
                template = template or parent.kind == k.CLASS_TEMPLATE
                class_offset = char_offset(parent.extent.start.offset)
                parent = parent.lexical_parent
            instances, invalid = instance_annotations(
                raw_texts[relative], char_offset(cursor.extent.start.offset),
                char_offset(cursor.location.offset), line_indexes[relative])
            if invalid or (instances and Counter(va for va, _ in instances) != Counter(vas)):
                errors.append(f'INSTANCE {relative}:{loc.line}: each selector must accompany one distinct VA annotation')
                instances = []
            elif len(vas) > 1 and not instances:
                errors.append(f'INSTANCE {relative}:{loc.line}: multiple VA annotations require concrete selectors')
            first = len(definitions)
            definitions.append(Definition(
                relative, loc.line, char_offset(cursor.extent.start.offset),
                closing + 1, _qualified(cursor, k),
                re.sub(r'\s+noexcept\b', '', cursor.type.spelling),
                sum(c.kind == k.PARM_DECL for c in cursor.get_children()),
                member, bool(is_inlined(cursor)),
                instances[0][0] if instances else (vas[0] if len(vas) == 1 else None),
                declaration_name(cursor),
                argument_types=tuple(c.type.spelling for c in cursor.get_children() if c.kind == k.PARM_DECL),
                const=(cursor.is_const_method()
                       if cursor.kind in {k.CXX_METHOD, k.CONVERSION_FUNCTION} else False),
                class_offset=class_offset, variadic=cursor.type.is_function_variadic(),
                instance=instances[0][1] if instances else "",
                return_type=('void' if cursor.kind in {k.CONSTRUCTOR, k.DESTRUCTOR}
                             else cursor.result_type.spelling),
                template=template,
                internal=(cursor.kind == k.FUNCTION_DECL
                          and cursor.storage_class == cindex.StorageClass.STATIC)))
            if instances:
                instance_requests.append((first, cursor.location.offset))
                extras = []
                for va, selector in instances[1:]:
                    extras.append(len(definitions))
                    instance_requests.append((len(definitions), cursor.location.offset))
                    definitions.append(replace(definitions[first], va=va, instance=selector))
                instance_groups.append((first, extras))
            return  # locals/calls are not definitions in another source file
        for child in cursor.get_children():
            visit(child)

    visit(tu.cursor)
    if instance_requests:
        definitions, failures = resolve_instances(
            definitions, instance_requests, unit, root, args)
        errors.extend(failures)
        extra_indices = set()
        for first, extras in instance_groups:
            definitions[first] = replace(definitions[first], additional_instances=tuple(
                (definitions[i].va, definitions[i].instance, definitions[i].mangled)
                for i in extras))
            extra_indices.update(extras)
        definitions = [d for i, d in enumerate(definitions) if i not in extra_indices]
    return definitions, errors, sorted(reached)


def unadmitted_sources(root: Path, admitted: set[str]) -> list[str]:
    return [path.relative_to(root).as_posix()
            for path in sorted((root / 'src').rglob('*'))
            if path.suffix.lower() in {'.c', '.cpp', '.cxx'}
            and path.relative_to(root).as_posix() not in admitted]


def collect(root: Path = ROOT, jobs: int = 4, fresh: bool = False):
    units = [u for u in manifest.units(root / 'config/units.toml')
             if u['source'].startswith('src/')]
    # Source-only edits reparse their TU and any TU including that source.
    # Header/config changes still invalidate every TU conservatively, including
    # inactive includes and header additions that change include resolution.
    admitted = {u['source'] for u in units}
    digest = hashlib.sha256(Path(__file__).read_bytes())
    digest.update(str(root.resolve()).encode())
    for dependency in ('scripts/homm3/core/clang.py', 'scripts/homm3/vc6/_source.py',
                       'scripts/homm3/manifest.py', 'scripts/homm3/retail_labels/source.py',
                       'scripts/homm3/core/compiler_profile.py', 'scripts/homm3/core/project.py'):
        digest.update((root / dependency).read_bytes())
    from homm3.core.compiler_profile import Profiles
    from homm3.core.project import Project
    profiles = Profiles(Project(root))
    mirror = profiles.mirror  # shared, constructed before workers
    from clang import cindex
    library = Path(cindex.conf.get_filename())
    digest.update(str(library).encode())
    if library.is_file():
        stat = library.stat()
        digest.update(f'{stat.st_size}:{stat.st_mtime_ns}'.encode())
    content = {}
    for base in ('src', 'include', 'config', 'vendor'):
        for path in sorted((root / base).rglob('*')):
            if path.is_file() and path.suffix.lower() in {'.h', '.hpp', '.inl', '.c', '.cpp', '.cxx', '.toml'}:
                relative = path.relative_to(root).as_posix()
                content[relative] = hashlib.sha256(path.read_bytes()).hexdigest()
                digest.update(relative.encode())
                if relative not in admitted:
                    digest.update(content[relative].encode())
    if mirror:
        for path in sorted(mirror.rglob('*')):
            if path.is_file():
                checksum = hashlib.sha256(path.read_bytes()).hexdigest()
                digest.update(path.relative_to(mirror).as_posix().encode())
                digest.update(checksum.encode())
                try:
                    content[path.relative_to(root).as_posix()] = checksum
                except ValueError:
                    pass  # External mirrors are covered by the shared key.
    cache = root / 'build/source-ownership/units'
    cache.mkdir(parents=True, exist_ok=True)
    key = digest.hexdigest()

    def cached_scan(unit):
        source = unit['source']
        path = cache / (hashlib.sha256(source.encode()).hexdigest() + '.json')
        if not fresh:
            try:
                saved = json.loads(path.read_text())
                if (saved['key'] == key and not saved['errors']
                        and source in saved['inputs']
                        and all(h is not None and content.get(p) == h
                                for p, h in saved['inputs'].items())):
                    return ([Definition(**r) for r in saved['definitions']],
                            saved['errors'], saved['reached'])
            except (OSError, ValueError, KeyError, TypeError, AttributeError):
                pass  # Disposable cache; a damaged entry must trigger a scan.
        definitions, errors, reached = scan_unit(unit, root, profiles=profiles)
        dependencies = {os.path.normpath(p) for p in [*reached, source]}
        input_hashes = {p: content.get(p) for p in sorted(dependencies)}
        saved = dict(key=key, definitions=[asdict(d) for d in definitions],
                     errors=errors, reached=reached, inputs=input_hashes)
        # Unknown dependencies cannot be reused without a content identity.
        if all(value is not None for value in input_hashes.values()):
            temporary = None
            try:
                with tempfile.NamedTemporaryFile(mode='w', dir=cache, delete=False) as stream:
                    temporary = Path(stream.name)
                    json.dump(saved, stream)
                os.replace(temporary, path)
            finally:
                if temporary is not None:
                    temporary.unlink(missing_ok=True)
        return definitions, errors, reached

    with ThreadPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(cached_scan, units))
        reached = {p for _, _, paths in results for p in paths}
        orphan_headers = [dict(source=p.relative_to(root).as_posix())
                          for p in sorted((root / 'include').rglob('*'))
                          if p.suffix.lower() in {'.h', '.hpp', '.inl'}
                          and p.relative_to(root).as_posix() not in reached]
        results.extend(pool.map(cached_scan, orphan_headers))
    unique = {}
    errors = [f'COVERAGE {relative}: source outside config/units.toml'
              for relative in unadmitted_sources(root, admitted)]
    reached = set()
    for definitions, failures, paths in results:
        errors.extend(failures)
        reached.update(paths)
        for d in definitions:
            unique[(d.file, d.offset, d.name, d.signature)] = d
    definitions = sorted(unique.values(), key=lambda d: (d.file, d.offset, d.signature))
    return definitions, errors, sorted(reached)


def active_stub_definitions(definitions: list[Definition], root: Path) -> list[str]:
    """Reject explicit placeholder comments inside AST-proven active bodies."""
    texts = {}
    errors = []
    # Consume literals before comments so a diagnostic string containing the
    # marker cannot turn a real implementation into a placeholder. These are
    # VC6 sources; raw C++11 string literals are outside their language profile.
    tokens = re.compile(r'''"(?:\\[\s\S]|[^"\\])*"|'(?:\\[\s\S]|[^'\\])*'|//[^\n]*|/\*[\s\S]*?\*/''')
    for definition in definitions:
        if definition.file not in texts:
            texts[definition.file] = (root / definition.file).read_text()
        body = texts[definition.file][definition.offset:definition.end]
        if '@stub' not in body:
            continue
        if any(re.match(r'(?:\/\/|\/\*)\s*@stub\b', token.group())
               for token in tokens.finditer(body)):
            errors.append(f'ACTIVE-STUB {definition.file}:{definition.line} '
                          f'{definition.name}: active definition contains an @stub placeholder; '
                          'recover its implementation or keep the reference under #if 0')
    return errors


class AuditResult(TypedDict):
    definitions: int
    violations: list[str]
    unpaired_generated_claims: list[str]
    reached: list[str]


def audit(root: Path = ROOT, jobs: int = 4, fresh: bool = False) -> AuditResult:
    from homm3.retail_labels.fragments import all_claims
    from homm3.core.project import Project
    project = Project(root)
    root = project.root
    image_base = project.specification["inputs"]["retail"]["image_base"]
    definitions, errors, reached = collect(root, jobs, fresh)
    errors.extend(active_stub_definitions(definitions, root))
    claims = all_claims(project.fragments)
    from homm3.match.status import load_baseline
    errors.extend(header_claim_ownership(
        definitions, claims, load_baseline(root / 'config/match_baseline.tsv'), image_base=image_base))
    errors.extend(claim_identity(definitions, claims, image_base=image_base))
    return {'definitions': len(definitions), 'violations': errors,
            'unpaired_generated_claims': unpaired_generated_claims(claims, image_base=image_base), 'reached': reached}


def header_claim_ownership(definitions: list[Definition], claims, banked=None, *, image_base=common.IMAGE_BASE) -> list[str]:
    """A matching inactive .cpp stub cannot substitute for the header VA."""
    by_name = defaultdict(set)
    for claim in claims:
        if claim.kind == 'func' and claim.channel.startswith('src-VA'):
            by_name[claim.name].add(claim.rva + image_base)
    # Removing an annotation also removes its source claim. The ledger still
    # knows that exact mangled identity, including when CUR is now unmatched.
    # Check it independently so the missing annotation cannot erase the gate's
    # own evidence that this header body needs a retail binding.
    for (_unit, name), row in (banked or {}).items():
        if row.rva is not None:
            by_name[name].add(row.rva + image_base)
    errors = []
    for d in claim_definitions(definitions):
        if not d.file.startswith('include/') or not d.mangled:
            continue
        addresses = by_name.get(d.mangled, set())
        if addresses and d.va not in addresses:
            errors.append(f'CLAIM_OWNER {d.file}:{d.line} {d.name}: canonical header body '
                          f'needs its VA annotation ({", ".join(hex(a) for a in sorted(addresses))})')
    return errors


def claim_identity(definitions: list[Definition], claims, *, image_base=common.IMAGE_BASE) -> list[str]:
    """Retaining an RVA under a raw placeholder is not retaining its identity."""
    from homm3.retail_labels.source import vc6_function_name
    by_address = defaultdict(set)
    for claim in claims:
        if claim.kind == 'func':
            by_address[claim.rva + image_base].add(claim.name)
    return [f'CLAIM_IDENTITY {d.file}:{d.line} {d.name}: {hex(d.va)} must name '
            f'{d.mangled!r}, extracted {sorted(by_address[d.va])!r}'
            for d in claim_definitions(definitions) if d.va is not None and d.mangled
            and vc6_function_name(d.mangled, by_address[d.va], Path(d.file).stem) is None]


def unpaired_generated_claims(claims, *, image_base=common.IMAGE_BASE) -> list[str]:
    """Report code-emission debt separately from written-source ownership.

    A library/implicit body may stop emitting after its callers change. Its
    source enrollment still identifies the retained retail body; do not force
    instantiation just to pass an ownership check. A written body has a direct
    VA obligation checked above and cannot borrow this generated enrollment.
    Anonymous initialization thunks acquire synthetic names deliberately and
    do not expect a named public.
    """
    from homm3.retail_labels.source import ANONYMOUS_COMPGEN_KINDS
    return [f'{c.unit}: {hex(c.rva + image_base)} '
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
    if result['unpaired_generated_claims']:
        print(f"[build] {len(result['unpaired_generated_claims'])} generated enrollments "
              "have no paired compiler body (code-emission debt)")
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
        print(f"{result['definitions']} definitions")
        print('\n'.join(result['violations']))
    return bool(result['violations'])


if __name__ == '__main__':
    raise SystemExit(main())
