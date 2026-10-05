"""Expand build-host exporter inputs from ordinary game translation units.

Clang selects definitions and expression boundaries using the project's Windows
profiles. The host compiler evaluates their initializer expressions; this tool
does not implement C++ arithmetic, layout, preprocessing or constructor execution.
Only explicit template directives are text-matched, never game declarations.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))

import clang.cindex as cx
from homm3.core.compiler_profile import Profiles
from homm3.core.project import Project
from homm3.retail_labels.data import _error_bodies


def qualified(cursor):
    names = []
    while cursor and cursor.kind != cx.CursorKind.TRANSLATION_UNIT:
        if cursor.spelling:
            names.append(cursor.spelling)
        cursor = cursor.semantic_parent
    return '::'.join(reversed(names))


def one(values, context):
    values = list(values)
    if len(values) != 1:
        raise ValueError(f'{context}: expected one definition/expression, got {len(values)}')
    return values[0]


def text(cursor):
    start, end = cursor.extent.start, cursor.extent.end
    if not start.file or start.file != end.file or start.offset >= end.offset:
        raise ValueError(f'{cursor.location}: expression has no contiguous source range')
    return Path(start.file.name).read_bytes()[start.offset:end.offset].decode('utf-8')


class Sources:
    def __init__(self, root, profiles=None):
        self.root = root
        self.profiles = profiles if profiles is not None else Profiles(Project(root))
        self.units = {}
        self.dependencies = set()

    def translation_unit(self, filename):
        if filename not in self.units:
            path = (self.root / filename).resolve()
            tu = cx.Index.create().parse(str(path), args=[
                *self.profiles.for_source(path), '-ferror-limit=0'],
                options=cx.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
            errors = [d for d in tu.diagnostics if d.severity >= cx.Diagnostic.Error]
            # VC6 permits constructs Clang rejects in unrelated function bodies.
            # Header/file-scope errors invalidate the TU; an erroneous body must
            # never supply an extracted declaration or recipe.
            bad = _error_bodies(tu, path, errors) if errors else set()
            if bad is None:
                raise ValueError('\n'.join(map(str, errors)))
            self.dependencies.add(path)
            self.dependencies.update(Path(i.include.name).resolve() for i in tu.get_includes())
            definitions = {}

            def visit(node):
                for child in node.get_children():
                    if not child.location.file or Path(child.location.file.name).resolve() != path:
                        continue
                    if child.is_definition() and child.kind in (
                            cx.CursorKind.VAR_DECL, cx.CursorKind.CXX_METHOD):
                        definitions.setdefault(qualified(child), []).append(child)
                    visit(child)
            visit(tu.cursor)
            self.units[filename] = tu, definitions, bad, errors
        return self.units[filename][0]

    def definition(self, filename, name):
        self.translation_unit(filename)
        _, definitions, bad, errors = self.units[filename]
        node = one(definitions.get(name, ()), f'{filename}: {name}')
        if any(node.extent.start.offset <= end and start <= node.extent.end.offset
               for start, end in bad):
            raise ValueError(f'{filename}: {name}: definition intersects an invalid body\n'
                             + '\n'.join(map(str, errors)))
        return node


def initializer(node):
    expressions = [c for c in node.get_children() if c.kind.is_expression()]
    if not expressions:
        raise ValueError(f'{node.location}: missing initializer')
    return expressions[-1]  # Earlier expressions are array dimensions.


def arguments(call):
    if call.kind != cx.CursorKind.CALL_EXPR:
        raise ValueError(f'{call.location}: expected constructor call, got {call.kind}')
    return [text(arg) for arg in call.get_arguments()]


def constructors(node):
    for child in node.walk_preorder():
        if child.kind == cx.CursorKind.CXX_NEW_EXPR:
            yield one((c for c in child.get_children() if c.kind == cx.CursorKind.CALL_EXPR),
                      f'{child.location}: allocated constructor')


FIXED = {
    'TRmgTreasureDef': 4, 'TRmgArtifactDef': 2, 'TRmgBlackBoxExperienceDef': 2,
    'TRmgBlackBoxGoldDef': 2, 'TRmgBlackBoxSpellsDef': 4, 'TRmgPrisonDef': 2,
    'TRmgResourceLumpDef': 4, 'TRmgScholarDef': 0, 'TRmgShrineDef': 2,
    'TRmgSpellScrollDef': 2, 'TRmgWitchHutDef': 0,
}
GROUPS = {
    frozenset(['TRmgBlackBoxCreatureDef']): 'CreatureBoxes',
    frozenset(['TRmgKeyTentDef']): 'KeyTents',
    frozenset(['TRmgMapDwellingDef']): 'Dwellings',
    frozenset(['TRmgQuestCreatureDef', 'TRmgQuestExperienceDef', 'TRmgQuestGoldDef']): 'Seers',
}


def validate_group(loop, group):
    if group not in ('KeyTents', 'Seers'):
        return
    body = one((c for c in loop.get_children() if c.kind == cx.CursorKind.COMPOUND_STMT),
               f'{loop.location}: {group} loop body')
    allocations = [(statement, list(constructors(statement))) for statement in body.get_children()]
    allocations = [(statement, calls) for statement, calls in allocations if calls]
    if group == 'Seers':
        first, calls = allocations.pop(0)
        if (first.kind not in (cx.CursorKind.FOR_STMT, cx.CursorKind.WHILE_STMT)
                or len(calls) != 1 or calls[0].spelling != 'TRmgQuestCreatureDef'):
            raise ValueError(f'{loop.location}: seer creature loop must precede direct rewards')
    permitted = ('TRmgKeyTentDef',) if group == 'KeyTents' else ('TRmgQuestExperienceDef', 'TRmgQuestGoldDef')
    for statement, calls in allocations:
        if (statement.kind not in (cx.CursorKind.UNEXPOSED_EXPR, cx.CursorKind.CALL_EXPR)
                or len(calls) != 1 or calls[0].spelling not in permitted):
            raise ValueError(f'{statement.location}: {group} requires unconditional direct appends')


def statements(body):
    """Walk unconditional lexical scopes without flattening control flow."""
    for statement in body.get_children():
        if statement.kind == cx.CursorKind.COMPOUND_STMT:
            yield from statements(statement)
        else:
            yield statement


def recipes(method):
    body = one((c for c in method.get_children() if c.kind == cx.CursorKind.COMPOUND_STMT),
               f'{method.location}: recipe body')
    emitted = []
    groups = []
    for statement in statements(body):
        calls = list(constructors(statement))
        if not calls:
            # Native setup declarations, pool resize/reset and loop counters
            # contain no recipe allocations. Their behavior belongs to Rust.
            setup_calls = [n for n in statement.walk_preorder() if n.kind == cx.CursorKind.CALL_EXPR]
            if any(c.spelling not in ('size', 'resize', 'operator[]') for c in setup_calls):
                raise ValueError(f'{statement.location}: unsupported recipe setup call')
            if statement.kind not in (cx.CursorKind.DECL_STMT, cx.CursorKind.IF_STMT):
                if len(setup_calls) != 1 or setup_calls[0].spelling != 'resize':
                    raise ValueError(f'{statement.location}: unsupported recipe setup statement')
            continue
        appends = [n for n in statement.walk_preorder()
                   if n.kind == cx.CursorKind.CALL_EXPR and n.spelling == 'push_back'
                   and any(c.kind == cx.CursorKind.MEMBER_REF_EXPR and c.spelling == 'm_objectGenerators'
                           for c in n.walk_preorder())]
        if len(appends) != len(calls):
            raise ValueError(f'{statement.location}: recipes must be appended to m_objectGenerators')
        if statement.kind in (cx.CursorKind.FOR_STMT, cx.CursorKind.WHILE_STMT):
            kinds = frozenset(c.spelling for c in calls)
            group = GROUPS.get(kinds)
            if group is None:
                raise ValueError(f'{statement.location}: unsupported dynamic recipe group {sorted(kinds)}')
            if group in ('CreatureBoxes', 'Dwellings') and len(calls) != 1:
                raise ValueError(f'{statement.location}: repeated allocation in {group}')
            if group == 'Seers' and sum(c.spelling == 'TRmgQuestCreatureDef' for c in calls) != 1:
                raise ValueError(f'{statement.location}: repeated seer creature allocation')
            validate_group(statement, group)
            groups.append(group)
            emitted.append(f'std::printf("TreasureRecipe::{group},");')
            continue
        if statement.kind not in (cx.CursorKind.UNEXPOSED_EXPR, cx.CursorKind.CALL_EXPR):
            raise ValueError(f'{statement.location}: conditional or unsupported fixed recipe')
        call = one(calls, f'{statement.location}: fixed recipe')
        args = arguments(call)
        if FIXED.get(call.spelling) != len(args):
            raise ValueError(f'{call.location}: unsupported recipe {call.spelling}/{len(args)}')
        emitted.append(f'emit{call.spelling.removeprefix("TRmg")}({", ".join(args)});')
    if sorted(groups) != sorted(GROUPS.values()):
        raise ValueError(f'{method.location}: missing or repeated dynamic recipe groups: {groups}')
    return '\n'.join(emitted)


def tent_values(method):
    calls = [c for c in constructors(method) if c.spelling == 'TRmgKeyTentDef']
    if not calls:
        raise ValueError(f'{method.location}: missing tent recipes')
    values = []
    for call in calls:
        args = arguments(call)
        if len(args) != 2:
            raise ValueError(f'{call.location}: unsupported tent constructor')
        values.append(args[1])
    return '{' + ', '.join(values) + '}'


def seer_rewards(method):
    emitted = []
    for call in constructors(method):
        if call.spelling not in ('TRmgQuestExperienceDef', 'TRmgQuestGoldDef'):
            continue
        args = arguments(call)
        if len(args) != 3:
            raise ValueError(f'{call.location}: unsupported seer constructor')
        kind = call.spelling.removeprefix('TRmgQuest').removesuffix('Def')
        emitted.append(f'RMG_SEER_REWARD({kind}, {args[1]}, {args[2]})')
    if not emitted:
        raise ValueError(f'{method.location}: missing seer rewards')
    return '\n'.join(emitted)


def expand(template, sources):
    output = []
    for line in template.splitlines(keepends=True):
        match = re.fullmatch(r'// @rmg (initializer|arguments|recipes|tent_values|seer_rewards) (\S+) (\S+)\n?', line)
        if match is None:
            output.append(line)
            continue
        operation, filename, name = match.groups()
        node = sources.definition(filename, name)
        if operation == 'initializer':
            if output and '[@count]' in output[-1]:
                count = node.type.get_array_size()
                if count <= 0:
                    raise ValueError(f'{node.location}: expected a bounded array')
                output[-1] = output[-1].replace('[@count]', f'[{count}]')
            value = text(initializer(node))
        elif operation == 'arguments':
            value = ', '.join(arguments(initializer(node)))
        else:
            value = {'recipes': recipes, 'tent_values': tent_values, 'seer_rewards': seer_rewards}[operation](node)
        output.append(value + ('\n' if line.endswith('\n') else ''))
    return ''.join(output)


def main():
    template, output = map(Path, sys.argv[1:])
    sources = Sources(ROOT)
    generated = expand(template.read_text(), sources)
    if '// @rmg ' in generated or '[@count]' in generated:
        raise ValueError('unrecognized extraction directive')
    from declarations import generate as declarations
    from constants import generate as constants
    header = declarations(sources) + constants(sources)
    output.with_name("declarations.h").write_text(header)
    output.write_text(generated)
    sources.dependencies.update(ROOT / 'config' / name for name in ('project.toml', 'units.toml'))
    sources.dependencies.update(Path(module.__file__).resolve()
                                for name, module in sys.modules.items()
                                if name.startswith('homm3.') and getattr(module, '__file__', None))
    for path in sorted(sources.dependencies):
        print(f'cargo:rerun-if-changed={path}')


if __name__ == '__main__':
    main()
