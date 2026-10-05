"""Build-only scalar aliases derived from native declarations and expressions.

These names are conveniences of the Rust data boundary, not new game data.
Constructor defaults, field widths and table bounds remain owned by C++.
Algorithm-local literals belong in the Rust implementation instead.
"""
import clang.cindex as cx

from extract import initializer, one, qualified, text


def integer_expression(expression):
    """Emit only arithmetic whose integer types agree on Windows and the host.

    In particular, a native sizeof expression must never be re-evaluated with
    host pointer sizes, and Windows long arithmetic must not become host long.
    Reject those expressions instead of silently changing their meaning.
    """
    arities = {
        cx.CursorKind.INTEGER_LITERAL: 0,
        cx.CursorKind.UNARY_OPERATOR: 1,
        cx.CursorKind.BINARY_OPERATOR: 2,
        cx.CursorKind.PAREN_EXPR: 1,
        cx.CursorKind.UNEXPOSED_EXPR: 1,
    }
    for node in expression.walk_preorder():
        children = list(node.get_children())
        if (node.kind not in arities or len(children) != arities[node.kind]
                or node.type.get_canonical().kind not in (cx.TypeKind.INT, cx.TypeKind.UINT)):
            raise ValueError(f'{node.location}: unsupported 32-bit integer constant expression')
        if node.kind == cx.CursorKind.UNARY_OPERATOR:
            if next(node.get_tokens()).spelling not in ('+', '-', '~'):
                raise ValueError(f'{node.location}: unsupported constant unary operator')
        elif node.kind == cx.CursorKind.BINARY_OPERATOR:
            lhs, rhs = children
            operators = [token.spelling for token in node.get_tokens()
                         if lhs.extent.end.offset <= token.extent.start.offset
                         and token.extent.end.offset <= rhs.extent.start.offset]
            if (len(operators) != 1
                    or operators[0] not in ('+', '-', '*', '/', '%', '&', '|', '^', '<<', '>>')):
                raise ValueError(f'{node.location}: unsupported constant binary operator')
    return text(expression)


class Native:
    def __init__(self, sources):
        # Load the ordinary TU with its real Windows compiler profile. Its
        # includes contain the inline treasure constructors and hero domain.
        sources.definition('src/rmg.cpp', 'TRmgGenerator::initializeObjectGenerators')
        self.tu, _, self.bad, self.errors = sources.units['src/rmg.cpp']
        self.nodes = list(self.tu.cursor.walk_preorder())

    def declaration(self, kind, name):
        node = one((n for n in self.nodes if n.kind == kind
                    and qualified(n) == name
                    and (kind not in (cx.CursorKind.CONSTRUCTOR,
                                      cx.CursorKind.FUNCTION_DECL,
                                      cx.CursorKind.CXX_METHOD) or n.is_definition())), name)
        # Invalid unrelated bodies may coexist in the VC6-profile TU; never
        # extract a value from one of those bodies.
        if (node.location.file and node.location.file.name == self.tu.spelling
                and any(node.extent.start.offset <= end and start <= node.extent.end.offset
                        for start, end in self.bad)):
            raise ValueError(f'{name}: selected declaration intersects an invalid body')
        return node

    def base_argument(self, constructor, index):
        node = self.declaration(cx.CursorKind.CONSTRUCTOR,
                                f'{constructor}::{constructor}')
        call = one((n for n in node.get_children()
                    if n.kind == cx.CursorKind.CALL_EXPR
                    and n.spelling == 'TRmgTreasureDef'),
                   f'{constructor}: treasure base initializer')
        args = list(call.get_arguments())
        if len(args) != 4:
            raise ValueError(f'{constructor}: expected four treasure arguments')
        return integer_expression(args[index])


def assignments(node):
    """Yield Clang-bounded assignment operands; do not parse declarations."""
    for child in node.walk_preorder():
        if child.kind != cx.CursorKind.BINARY_OPERATOR:
            continue
        operands = list(child.get_children())
        if len(operands) != 2:
            continue
        lhs, rhs = operands
        # Tokenize only the operator gap, so nested indices/expressions cannot
        # make a comparison look like an assignment.
        between = [t.spelling for t in child.get_tokens()
                   if lhs.extent.end.offset <= t.extent.start.offset
                   and t.extent.end.offset <= rhs.extent.start.offset]
        if between == ['=']:
            yield lhs, rhs


def array_count(node):
    count = node.type.get_array_size()
    if count <= 0:
        raise ValueError(f'{node.location}: expected a bounded array')
    return count


def generate(sources):
    native = Native(sources)
    values = {}
    defaults = {
        'ARTIFACT': 'TRmgArtifactDef',
        'CREATURE': 'TRmgBlackBoxCreatureDef',
        'EXPERIENCE_BOX': 'TRmgBlackBoxExperienceDef',
        'GOLD_BOX': 'TRmgBlackBoxGoldDef',
        'SPELL_BOX': 'TRmgBlackBoxSpellsDef',
        'KEY_TENT': 'TRmgKeyTentDef',
        'DWELLING': 'TRmgDwellingDef',
        'PRISON': 'TRmgPrisonDef',
        'SCHOLAR': 'TRmgScholarDef',
        'QUEST': 'TRmgQuestExperienceDef',
        'SHRINE': 'TRmgShrineDef',
        'WITCH_HUT': 'TRmgWitchHutDef',
        'SCROLL': 'TRmgSpellScrollDef',
    }
    for name, constructor in defaults.items():
        suffix = '_DENSITY' if name.endswith('_BOX') else '_REWARD_DENSITY'
        values[f'RMG_{name}{suffix}'] = native.base_argument(constructor, 3)
    for name in ('SCHOLAR', 'WITCH_HUT'):
        values[f'RMG_{name}_REWARD_VALUE'] = native.base_argument(defaults[name], 2)

    # Both non-creature seer recipes share the same density in the native
    # implementation. Keep that relationship checked by the host compiler.
    quest_gold_density = native.base_argument('TRmgQuestGoldDef', 3)

    field = native.declaration(cx.CursorKind.FIELD_DECL,
                               'TRmgBorderConnection::m_guardColor')
    if not field.is_bitfield() or field.get_bitfield_width() <= 0:
        raise ValueError('border guard colour must remain a bounded bitfield')
    values['RMG_BORDER_COLOR_BITS'] = str(field.get_bitfield_width())
    offsets = sources.definition('src/rmg.cpp',
                                 'TRmgGenerator::placeMonolithBorderGuard::offsets')
    values['RMG_PORTAL_BORDER_OFFSET_COUNT'] = str(array_count(offsets))

    spell_count = one((n for n in native.nodes
                       if n.kind == cx.CursorKind.ENUM_CONSTANT_DECL
                       and n.spelling == 'NUM_SPELLS'
                       and n.semantic_parent.semantic_parent.spelling == 'hero'),
                      'hero::NUM_SPELLS')
    values['HERO_SPELL_COUNT'] = str(spell_count.enum_value)
    for category in ('0', '4', '5'):
        entry = one((n for n in native.nodes
                     if n.kind == cx.CursorKind.ENUM_CONSTANT_DECL
                     and n.spelling == f'SLOT_CATEGORY_{category}'
                     and n.semantic_parent.semantic_parent.spelling == 'TObjectType'),
                    f'TObjectType::SLOT_CATEGORY_{category}')
        values[f'OBJECT_SLOT_CATEGORY_{category}'] = str(entry.enum_value)

    constructor = native.declaration(cx.CursorKind.CONSTRUCTOR,
                                     'TRmgGenerator::TRmgGenerator')
    limits = {}
    for scope in ('Zone', 'Map'):
        name = f'g_rmg{scope}ObjectLimits'
        default = one((rhs for lhs, rhs in assignments(constructor)
                       if lhs.kind == cx.CursorKind.ARRAY_SUBSCRIPT_EXPR
                       and rhs.kind == cx.CursorKind.INTEGER_LITERAL
                       and any(n.kind == cx.CursorKind.DECL_REF_EXPR and n.spelling == name
                               for n in lhs.walk_preorder())), f'{name}: default assignment')
        limits[scope] = integer_expression(default)
    values['RMG_DEFAULT_OBJECT_LIMIT'] = limits['Zone']
    vial = sources.definition('src/rmg.cpp', 'g_rmgArtifactVialOfDragonBlood')
    values['RMG_ARTIFACT_VIAL_OF_DRAGON_BLOOD'] = integer_expression(initializer(vial))

    rows = ['// Generated from native constructor arguments, dimensions and assignments.',
            'enum ERmgDerivedConstants {']
    rows.extend(f'    {name} = {value},' for name, value in values.items())
    rows.extend(['};',
                 f'static_assert(RMG_QUEST_REWARD_DENSITY == ({quest_gold_density}),',
                 '              "native seer reward densities differ");',
                 f'static_assert(RMG_DEFAULT_OBJECT_LIMIT == ({limits["Map"]}),',
                 '              "native map and zone object limits differ");'])
    return '\n'.join(rows) + '\n'
