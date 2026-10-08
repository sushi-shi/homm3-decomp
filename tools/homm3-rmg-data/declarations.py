"""Build-only declaration closure selected from the ordinary game AST.

No game declaration or enum value is maintained here. Enums use Clang's
evaluated values (including the owning TU's preprocessing); plain records keep
their source fields. Only the two artifact adapters need native constructors.
"""
from pathlib import Path

import clang.cindex as cx

from extract import qualified, text


DOMAIN_TYPES = (
    'TTownType', 'TTerrainType', 'TArtifact', 'ERandomMapResult',
    'EMapDimension', 'ETileDirection', 'EGameResource', 'TAdventureObjectType',
    'EObjectMaskFrame', 'TSpellSchool', 'EKeyColor', 'TSecondarySkill',
    'EQuestType', 'TSeerRewardType', 'EMapFormatVersion',
    'ECartographerType', 'type_creature_bank_type',
)
RECORDS = (
    'TRandomMapRequest', 'TRmgTerrainPatternEntry', 'TRmgTerrainTransitionEntry',
    'TAdvObjectNameRow', 'TCreatureTypeTraits', 'TSpellTraits', 'THeroTraits',
    'TCombinationArtifact', 'TArtifactSlotMask',
)
CONSTRUCTED_RECORDS = frozenset(('TCombinationArtifact', 'TArtifactSlotMask'))
CONSTANTS = ('ADVENTURE_OBJECT_TRAIT_COUNT', 'TOWN_TYPE_COUNT', 'TOWN_TYPE_ROE_COUNT')
DECLARATIONS = frozenset((cx.CursorKind.ENUM_DECL, cx.CursorKind.STRUCT_DECL,
                         cx.CursorKind.CLASS_DECL, cx.CursorKind.TYPEDEF_DECL))


class Declarations:
    def __init__(self, sources):
        self.sources = sources
        self.named = {}
        self.constants = {}
        self.macros = {}
        self.emitted = set()
        self.pending = set()
        self.output = []

    def index(self, tu):
        # Roots are global declarations. Dependencies are followed through
        # referenced cursors, so similarly named members never become roots.
        self.index_scope(tu.cursor)

    def index_scope(self, scope):
        for node in scope.get_children():
            if not self.is_game(node):
                continue
            if node.kind == cx.CursorKind.NAMESPACE and node.is_anonymous():
                self.index_scope(node)
                continue
            if node.kind in DECLARATIONS and node.is_definition():
                name = qualified(node)
                self.named.setdefault(name, {}).setdefault(self.identity(node), []).append(node)
            if node.kind == cx.CursorKind.MACRO_DEFINITION:
                self.macros.setdefault(node.spelling, []).append(node)
            if node.kind == cx.CursorKind.ENUM_DECL:
                for item in node.get_children():
                    if item.kind == cx.CursorKind.ENUM_CONSTANT_DECL:
                        self.constants.setdefault(item.spelling, {}).setdefault(self.identity(node), []).append(node)

    def is_game(self, node):
        if not node.location.file:
            return False
        path = Path(node.location.file.name).resolve()
        return any(path.is_relative_to(self.sources.root / folder) for folder in ('src', 'include'))

    def select(self, name):
        nodes = self.named.get(name, {})
        if not nodes:
            raise ValueError(f'missing native declaration: {name}')
        if len(nodes) != 1:
            raise ValueError(f'ambiguous native declaration: {name}')
        return self.consistent(name, next(iter(nodes.values())))

    @staticmethod
    def identity(node):
        return (str(node.location.file), node.extent.start.offset)

    @classmethod
    def signature(cls, node):
        if node.kind == cx.CursorKind.ENUM_DECL:
            return (node.enum_type.spelling, tuple(
                (child.spelling, child.enum_value) for child in node.get_children()
                if child.kind == cx.CursorKind.ENUM_CONSTANT_DECL))
        if node.kind == cx.CursorKind.TYPEDEF_DECL:
            return node.underlying_typedef_type.get_canonical().spelling
        fields = []
        for child in node.get_children():
            if child.kind == cx.CursorKind.FIELD_DECL:
                fields.append(('field', child.spelling, child.type.get_canonical().spelling,
                               child.get_field_offsetof(), child.type.get_size(),
                               child.get_bitfield_width() if child.is_bitfield() else None))
            elif child.kind in (cx.CursorKind.STRUCT_DECL, cx.CursorKind.UNION_DECL):
                fields.append(('record', child.spelling, cls.signature(child)))
            elif child.kind == cx.CursorKind.CXX_BASE_SPECIFIER:
                fields.append(('base', child.type.get_canonical().spelling, child.type.get_size()))
            elif child.kind == cx.CursorKind.CONSTRUCTOR:
                fields.append(('constructor', child.type.spelling))
            elif child.kind in (cx.CursorKind.CXX_METHOD, cx.CursorKind.DESTRUCTOR) and child.is_virtual_method():
                fields.append(('virtual', child.spelling, child.type.spelling))
        return (node.type.get_size(), node.type.get_align(), tuple(fields))

    @classmethod
    def consistent(cls, name, nodes):
        first = nodes[0]
        expected = cls.signature(first)
        if any(cls.signature(node) != expected for node in nodes[1:]):
            raise ValueError(f'native declaration differs across TU profiles: {name}')
        return first

    def constant(self, name):
        nodes = self.constants.get(name, {})
        if len(nodes) != 1:
            raise ValueError(f'missing or ambiguous native constant declaration: {name}')
        self.emit(self.consistent(name, next(iter(nodes.values()))))

    def macro(self, name):
        definitions = self.macros.get(name, ())
        spellings = {tuple(token.spelling for token in node.get_tokens()) for node in definitions}
        if len(spellings) != 1:
            raise ValueError(f'missing or ambiguous native macro: {name}')
        tokens, = spellings
        # Only an object-like integer macro is needed. Reject function-like
        # macros and expressions with untracked declaration dependencies.
        if len(tokens) != 2 or not tokens[1].isdigit():
            raise ValueError(f'unsupported native integer macro: {name}')
        for node in definitions:
            self.sources.dependencies.add(Path(node.location.file.name).resolve())
        self.output.append(f'#define {name} {tokens[1]}\n')

    def dependencies(self, node):
        for child in node.walk_preorder():
            if child.kind not in (cx.CursorKind.TYPE_REF, cx.CursorKind.TEMPLATE_REF,
                                  cx.CursorKind.DECL_REF_EXPR, cx.CursorKind.CXX_BASE_SPECIFIER):
                continue
            target = child.referenced
            if target is None:
                raise ValueError(f'{child.location}: unresolved declaration dependency')
            if target.kind == cx.CursorKind.ENUM_CONSTANT_DECL:
                target = target.semantic_parent
            if qualified(target) == 'std::bitset':
                continue  # Host implementation, never the VC6 container layout.
            if not self.is_game(target):
                raise ValueError(f'{child.location}: unsupported external dependency {qualified(target)}')
            if target.kind not in DECLARATIONS:
                # Constructor field initializers and parameter/local references
                # are owned by the record already being emitted.
                if target.kind in (cx.CursorKind.FIELD_DECL, cx.CursorKind.PARM_DECL):
                    continue
                raise ValueError(f'{child.location}: unsupported dependency {qualified(target)}')
            definition = target.get_definition() or target
            self.emit(definition)

    def emit(self, node):
        key = self.identity(node)
        if key in self.emitted:
            return
        if key in self.pending:
            raise ValueError(f'{node.location}: cyclic declaration dependency {qualified(node)}')
        parent = node.semantic_parent
        while parent.kind == cx.CursorKind.NAMESPACE and parent.is_anonymous():
            parent = parent.semantic_parent
        if parent.kind != cx.CursorKind.TRANSLATION_UNIT:
            raise ValueError(f'{node.location}: unsupported scoped declaration {qualified(node)}')
        # Only selected roots/dependencies need an unambiguous projected name.
        # Ordinary game TUs may contain unrelated repeated typedefs or private
        # namespace declarations that are irrelevant to this header.
        candidates = self.named.get(qualified(node), {})
        if any(candidate != key for candidate in candidates):
            raise ValueError(f'{node.location}: ambiguous native declaration: {qualified(node)}')
        if key in candidates:
            self.consistent(qualified(node), [node, *candidates[key]])
        self.pending.add(key)
        name = node.spelling
        if node.kind == cx.CursorKind.ENUM_DECL:
            if node.is_scoped_enum():
                raise ValueError(f'{node.location}: scoped enum is not a scalar export domain')
            if node.enum_type.kind not in (cx.TypeKind.INT, cx.TypeKind.UINT):
                raise ValueError(f'{node.location}: unsupported enum storage {node.enum_type.spelling}')
            items = [f'    {item.spelling} = {item.enum_value},'
                     for item in node.get_children() if item.kind == cx.CursorKind.ENUM_CONSTANT_DECL]
            if not items:
                raise ValueError(f'{node.location}: empty enum declaration')
            # An anonymous enum's Clang spelling is a diagnostic, not C++ text.
            tag = '' if node.is_anonymous() else name
            rendered = f'enum {tag} {{\n' + '\n'.join(items) + '\n};'
        elif node.kind == cx.CursorKind.TYPEDEF_DECL:
            self.dependencies(node)
            rendered = text(node) + ';'
        elif name in CONSTRUCTED_RECORDS:
            self.dependencies(node)
            rendered = '#ifndef HOMM3_RMG_BINDGEN\n' + text(node) + ';\n#endif'
        elif node.kind in (cx.CursorKind.STRUCT_DECL, cx.CursorKind.CLASS_DECL):
            fields = []
            for child in node.get_children():
                if child.kind == cx.CursorKind.CXX_BASE_SPECIFIER:
                    raise ValueError(f'{child.location}: plain export record cannot have bases')
                if child.kind == cx.CursorKind.CONSTRUCTOR and name != 'TRandomMapRequest':
                    raise ValueError(f'{child.location}: plain export record cannot discard constructors')
                if child.kind in (cx.CursorKind.CXX_METHOD, cx.CursorKind.DESTRUCTOR) and child.is_virtual_method():
                    raise ValueError(f'{child.location}: plain export record cannot have virtual methods')
                if child.kind == cx.CursorKind.FIELD_DECL or (
                        child.kind in (cx.CursorKind.UNION_DECL, cx.CursorKind.STRUCT_DECL)
                        and child.is_anonymous()):
                    self.dependencies(child)
                    fields.append(text(child) + ';')
            if not fields:
                raise ValueError(f'{node.location}: empty export record')
            rendered = f'struct {name} {{\n' + '\n'.join(fields) + '\n};'
        else:
            raise ValueError(f'{node.location}: unsupported declaration {node.kind}')
        self.pending.remove(key)
        self.emitted.add(key)
        self.sources.dependencies.add(Path(node.location.file.name).resolve())
        origin = Path(node.location.file.name).resolve().relative_to(self.sources.root)
        self.output.append(f'// {origin}:{node.location.line}\n{rendered}\n')


def generate(sources):
    """Return declarations.h, sharing Sources' parsed TUs and dependencies."""
    declarations = Declarations(sources)
    for filename, anchor in (
            ('src/rmg.cpp', 'TRmgGenerator::initializeObjectGenerators'),
            ('src/rmg_terrain.cpp', 'g_rmgTerrainPatterns'),
            ('src/game.cpp', 'g_creatureGenerator1Types')):
        sources.definition(filename, anchor)
        declarations.index(sources.units[filename][0])
    # Key colours remain source-owned even when no extracted TU uses that enum.
    declarations.index(sources.translation_unit('include/keycolor.h'))
    for name in DOMAIN_TYPES + RECORDS:
        declarations.emit(declarations.select(name))
    for name in CONSTANTS:
        declarations.constant(name)
    declarations.macro('NUM_RESOURCES')
    for name, origins in declarations.named.items():
        if name.startswith('ERmg') and any(nodes[0].kind == cx.CursorKind.ENUM_DECL for nodes in origins.values()):
            declarations.emit(declarations.select(name))
    return ('// Generated from ordinary game declarations; do not edit.\n'
            '#ifndef HOMM3_RMG_GENERATED_DECLARATIONS_H\n'
            '#define HOMM3_RMG_GENERATED_DECLARATIONS_H\n'
            '#include "homm3_int.h"\n'
            '#ifndef HOMM3_RMG_BINDGEN\n#include <bitset>\n#endif\n\n'
            + '\n'.join(declarations.output) + '\n#endif\n')
