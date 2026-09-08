"""Negative controls for definition ownership, order and exact filters."""
import tempfile
import unittest
from pathlib import Path

from homm3.match.source_ownership import Definition, Origin, compare, read_filter


def definition(name='Widget::draw', file='include/widget.h', line=20,
               signature='void ()', inline=True):
    return Definition(file, line, line, line + 1, name, signature, 0, True,
                      inline, None, '')


def origin(name='Widget::draw', file='widget.h', line=100):
    return Origin(file, name, line, 1, 'adventure.obj', '0x1000')


class OwnershipTest(unittest.TestCase):
    def test_normalized_operation_keeps_owner_signature_and_duplicate_checks(self):
        from dataclasses import replace
        d = definition(name='Widget::getValue')
        o = replace(origin(name='Widget::Get_Value'), argument_types=())
        self.assertEqual(compare([d], [o], {}, {})[0], [])
        wrong_owner = replace(d, file='include/other.h')
        self.assertTrue(compare([wrong_owner], [o], {}, {})[0][0].startswith('OWNER '))
        wrong_signature = replace(d, parameters=1, argument_types=('int',))
        self.assertTrue(compare([wrong_signature], [o], {}, {})[0][0].startswith('SIGNATURE '))
        duplicate = replace(d, line=30, offset=30, name='Widget::get_value')
        self.assertTrue(any(e.startswith('DUPLICATE ') for e in compare([d, duplicate], [o], {}, {})[0]))
        distinct_type = replace(d, name='widget::getValue')
        self.assertTrue(compare([distinct_type], [o], {}, {})[0][0].startswith('WIN_ONLY '))

    def test_normalized_name_collision_requires_source_identity(self):
        d = definition(name='Widget::getValue')
        errors, _ = compare([d], [origin(name='Widget::GetValue'),
                                   origin(name='Widget::get_value', line=200)], {}, {})
        self.assertTrue(any(e.startswith('AMBIGUOUS ') for e in errors))

    def test_source_location_accepts_comma_without_crossing_a_body(self):
        from homm3.match.source_ownership import origin_hint
        for suffix in (', dc 0xa8b74. PC adds the version.', '. PC adds the version.', ''):
            raw = '// E:\\gamedcs\\game.cpp:3266' + suffix + '\nvoid load() {}\n'
            self.assertEqual(origin_hint(raw, raw.index('load()')),
                             ('game.cpp', 3266, ''))
            raw += '\nvoid next() {}\n'
            self.assertEqual(origin_hint(raw, raw.index('next()')), ('', 0, ''))
        malformed = '// E:\\gamedcs\\game.cpp:3266abc\nvoid load() {}\n'
        self.assertEqual(origin_hint(malformed, malformed.index('load()')), ('', 0, ''))

    def test_private_class_copies_are_distinct_only_in_their_dc_modules(self):
        from unittest import mock
        from homm3.cleanliness.board import _dc_local_classes
        name = "`anonymous namespace'::StringOwner::get"
        dc = [origin(name=name, file=f) for f in ('hero.cpp', 'spell.cpp')]
        sources = [(Path(f), 'namespace { class StringOwner {}; }')
                   for f in ('hero.cpp', 'spell.cpp', 'other.cpp', 'owner.h')]
        with mock.patch('homm3.match.source_ownership.read_dc', return_value=dc):
            allowed = _dc_local_classes(sources)
        self.assertEqual(allowed[Path('hero.cpp')], {'StringOwner'})
        self.assertEqual(allowed[Path('spell.cpp')], {'StringOwner'})
        self.assertEqual(allowed[Path('other.cpp')], set())
        self.assertNotIn(Path('owner.h'), allowed)

    def test_private_class_evidence_does_not_admit_wrong_scope_or_duplicate(self):
        from unittest import mock
        from homm3.cleanliness.board import _dc_local_classes
        dc = [origin(name="`anonymous namespace'::StringOwner::get", file='hero.cpp')]
        cases = [
            'class StringOwner {};',
            'namespace unrelated { class StringOwner {}; }',
            'namespace {} class StringOwner {};',
            'namespace { namespace unrelated { class StringOwner {}; } }',
            'namespace { void f() { class StringOwner {}; } }',
            'namespace { class StringOwner {}; class StringOwner {}; }',
        ]
        for code in cases:
            with self.subTest(code=code), mock.patch(
                    'homm3.match.source_ownership.read_dc', return_value=dc):
                self.assertEqual(_dc_local_classes([(Path('hero.cpp'), code)]),
                                 {Path('hero.cpp'): frozenset()})

    def test_unemitted_copy_constructor_does_not_borrow_pointer_constructor_line(self):
        from dataclasses import replace
        pointer = replace(definition(name='SmartPtr::SmartPtr<T>', file='include/smartptr.h'),
                          parameters=1, argument_types=('T *',), signature='void (T *)')
        copy = replace(pointer, line=30, offset=30,
                       argument_types=('const SmartPtr<T> &',),
                       signature='void (const SmartPtr<T> &)')
        proc = replace(origin(name='SmartPtr<char>::SmartPtr<char>', file='smartptr.h'),
                       argument_types=('char *',))
        declaration = replace(proc, file='', line=0, offset='', module='',
                              argument_types=('const SmartPtr<char> &',),
                              declaration_only=True, type_index=0x1234)
        errors, counts = compare([pointer, copy], [proc, declaration], {}, {})
        self.assertEqual(counts, {'same_file': 1, 'unlocated': 1})
        self.assertEqual(len(errors), 1)
        self.assertTrue(errors[0].startswith('UNLOCATED include/smartptr.h:30 '))
        self.assertIn('type 0x1234', errors[0])
        # Keeping the missing copy body distinct must not permit two bodies
        # for the pointer constructor that really does have a source line.
        duplicate = replace(pointer, line=40, offset=40)
        errors, _ = compare([pointer, copy, duplicate], [proc, declaration], {}, {})
        self.assertTrue(any(e.startswith('DUPLICATE ') for e in errors))

    def test_declaration_only_counterpart_cannot_be_filtered_as_windows_only(self):
        from dataclasses import replace
        d = definition()
        declaration = replace(origin(), file='', line=0, offset='',
                              argument_types=(), declaration_only=True, type_index=0x1234)
        key = (d.file, d.name, d.signature)
        errors, _ = compare([d], [declaration], {}, {key: 'No emitted procedure'})
        self.assertTrue(any('hides a CodeView counterpart' in e for e in errors))
        # The file/line DC filter cannot manufacture a source identity for a
        # field-list declaration, either.
        errors, _ = compare([d], [declaration],
                            {('', declaration.name, '0'): 'No emitted procedure'}, {})
        self.assertTrue(any('stale dc_only' in e for e in errors))

    def test_field_list_declarations_preserve_overloads_without_inventing_lines(self):
        from dataclasses import replace
        from homm3.match.source_ownership import declaration_origins
        class Types:
            records = {1: {}, 2: {}}
            def get(self, index):
                return {
                    0: dict(kind='primitive'),
                    1: dict(kind='class', name='SmartPtr<char>', fields=10),
                    2: dict(kind='class', name='SmartPtr<char>', forward=True),
                    20: dict(entries=[dict(type=30, attributes=3),
                                     dict(type=31, attributes=3)]),
                    30: dict(kind='function', arguments=40),
                    31: dict(kind='function', arguments=41),
                    32: dict(kind='function', arguments=42),
                    40: dict(types=[50]), 41: dict(types=[51]), 42: dict(types=[]),
                }[index]
            def fields(self, index):
                return [dict(kind='overloads', name='SmartPtr<char>', type=20),
                        dict(kind='method', name='~SmartPtr<char>', type=32, attributes=0x103)]
            def declaration(self, index):
                return {50: 'char *', 51: 'const SmartPtr<char> &'}[index]
        proc = replace(origin(name='SmartPtr<char>::SmartPtr<char>', file='smartptr.h'),
                       argument_types=('char *',))
        missing = declaration_origins(Types(), [proc])
        self.assertEqual(len(missing), 2)
        copy, destructor = missing
        self.assertEqual(copy.argument_types, ('const SmartPtr<char> &',))
        self.assertFalse(copy.generated)
        self.assertTrue(destructor.generated)
        self.assertTrue(all(o.declaration_only and not o.file and not o.line
                            and not o.offset for o in missing))
        # A second emission or type-record spelling cannot create a missing
        # definition when the same formal identity already has a procedure.
        copy_proc = replace(copy, file='smartptr.h', line=90, offset='0x2000',
                            declaration_only=False)
        self.assertEqual(len(declaration_origins(Types(), [proc, copy_proc])), 1)

    def test_adapter_cannot_borrow_the_canonical_helpers_dc_identity(self):
        from dataclasses import replace
        a = replace(definition(name='helper', file='src/widget.cpp'),
                    parameters=1, argument_types=('unsigned long',), signature='int (unsigned long)')
        b = replace(a, line=30, offset=30, argument_types=('int',), signature='int (int)')
        o = replace(origin(name='helper', file='widget.cpp'), argument_types=('unsigned long',))
        errors, counts = compare([a, b], [o], {}, {})
        self.assertEqual(counts['duplicate'], 1)
        self.assertTrue(any(e.startswith('DUPLICATE ') for e in errors))
        # An explicit source bridge establishes identity; it cannot waive a
        # second physical definition of that same function.
        errors, _ = compare([a, replace(b, dc_offset=o.offset)], [o], {}, {})
        self.assertTrue(any(e.startswith('DUPLICATE ') for e in errors))

    def test_real_overloads_on_the_same_dc_line_remain_distinct(self):
        from dataclasses import replace
        a = replace(definition(name='helper', file='src/widget.cpp'),
                    parameters=1, argument_types=('unsigned long',), signature='int (unsigned long)')
        b = replace(a, line=30, offset=30, argument_types=('int',), signature='int (int)')
        o = replace(origin(name='helper', file='widget.cpp'), argument_types=('unsigned long',))
        other = replace(o, offset='0x2000', argument_types=('int',))
        self.assertEqual(compare([a, b], [o, other], {}, {})[0], [])

    def test_repeated_dc_emissions_do_not_authorize_duplicate_bodies(self):
        from dataclasses import replace
        a = definition()
        b = replace(a, line=30, offset=30)
        o = origin()
        repeated = replace(o, module='second.obj', offset='0x2000')
        self.assertEqual(compare([a], [o, repeated], {}, {})[0], [])
        errors, _ = compare([a, b], [o, repeated], {}, {})
        self.assertTrue(any(e.startswith('DUPLICATE ') for e in errors))

    def test_renamed_source_identity_stays_attached_and_is_not_a_waiver(self):
        from dataclasses import replace
        from homm3.match.source_ownership import origin_hint
        raw = '// Original: Widget::Draw; Widget.h:100, dc 0x1000.\nvoid draw() {}\n'
        hint = origin_hint(raw, raw.index('draw()'))
        self.assertEqual(hint, ('widget.h', 100, '0x1000'))
        d = replace(definition(file='src/wrong.cpp'), origin_file=hint[0],
                    origin_line=hint[1], dc_offset=hint[2])
        errors, _ = compare([d], [origin(name='Widget::Draw')], {}, {})
        self.assertTrue(any(e.startswith('OWNER ') for e in errors))
        raw += 'void next() {}\n'
        self.assertEqual(origin_hint(raw, raw.index('next()')), ('', 0, ''))

    def test_generated_member_is_not_a_written_source_owner(self):
        from dataclasses import replace
        d = definition(name='Widget::~Widget')
        o = replace(origin(name=d.name, file='caller.cpp'), generated=True)
        errors, counts = compare([d], [o], {}, {})
        self.assertEqual(counts, {'generated': 1})
        self.assertTrue(errors[0].startswith('GENERATED '))
        key = (d.file, d.name, d.signature)
        self.assertEqual(compare([d], [o], {}, {key: 'Reviewed new Windows destructor'})[0], [])

    def test_generated_type_metadata_keeps_names_and_overloads_distinct(self):
        from homm3.match.source_ownership import generated_members
        class Types:
            records = {1: {}, 2: {}}
            def get(self, index):
                return {
                    1: dict(kind='class', name='Widget', fields=10),
                    2: dict(kind='class', name='Widget', forward=True),
                    20: dict(entries=[dict(type=30, attributes=0x103),
                                     dict(type=31, attributes=3)]),
                }[index]
            def fields(self, index):
                return [dict(kind='method', name='~Widget', type=40, attributes=0x103),
                        dict(kind='method', name='clear', type=40, attributes=3),
                        dict(kind='overloads', name='Widget', type=20)]
        self.assertEqual(generated_members(Types()),
                         {('Widget::~Widget', 40), ('Widget::Widget', 30)})

    def test_header_origin_is_not_compiland_ownership(self):
        errors, _ = compare([definition()], [origin()], {}, {})
        self.assertEqual(errors, [])

    def test_inline_without_va_is_still_checked(self):
        errors, _ = compare([definition(file='src/adventure.cpp')], [origin()], {}, {})
        self.assertTrue(any(e.startswith('OWNER ') for e in errors))

    def test_dc_line_order_is_independent_of_retail_addresses(self):
        definitions = [definition(name='Widget::b', line=20),
                       definition(name='Widget::a', line=30)]
        origins = [origin(name='Widget::a', line=100), origin(name='Widget::b', line=200)]
        errors, _ = compare(definitions, origins, {}, {})
        self.assertTrue(any(e.startswith('ORDER ') for e in errors))

    def test_ordinary_cpp_claims_use_retail_order_without_waiving_ownership(self):
        from dataclasses import replace
        a = replace(definition(name='Widget::a', file='src/widget.cpp', inline=False), va=0x401000)
        b = replace(definition(name='Widget::b', file='src/widget.cpp', line=30, inline=False), va=0x402000)
        origins = [origin(name=a.name, file='widget.cpp', line=200),
                   origin(name=b.name, file='widget.cpp', line=100)]
        self.assertEqual(compare([a, b], origins, {}, {})[0], [])
        errors, _ = compare([replace(a, file='src/wrong.cpp'), b], origins, {}, {})
        self.assertTrue(any(e.startswith('OWNER ') for e in errors))

    def test_ordinary_claim_does_not_advance_inline_source_order(self):
        from dataclasses import replace
        a = definition(name='Widget::a', file='src/widget.cpp', line=10)
        b = replace(definition(name='Widget::b', file=a.file, line=20, inline=False), va=0x401000)
        c = replace(definition(name='Widget::c', file=a.file, line=30), va=0x409000)
        origins = [origin(name=a.name, file='widget.cpp', line=100),
                   origin(name=b.name, file='widget.cpp', line=300),
                   origin(name=c.name, file='widget.cpp', line=200)]
        self.assertEqual(compare([a, b, c], origins, {}, {})[0], [])
        errors, _ = compare([c, b, a], origins, {}, {})
        self.assertTrue(any(e.startswith('ORDER ') for e in errors))

    def test_unclaimed_ordinary_helpers_still_follow_source_order(self):
        a = definition(name='Widget::a', file='src/widget.cpp', inline=False)
        b = definition(name='Widget::b', file=a.file, line=30, inline=False)
        origins = [origin(name=a.name, file='widget.cpp', line=200),
                   origin(name=b.name, file='widget.cpp', line=100)]
        self.assertTrue(any(e.startswith('ORDER ') for e in compare([a, b], origins, {}, {})[0]))

    def test_win_filter_cannot_hide_misplaced_dc_function(self):
        d = definition(file='include/other.h')
        key = (d.file, d.name, d.signature)
        errors, _ = compare([d], [origin()], {}, {key: 'not a legitimate exclusion'})
        self.assertTrue(any('hides a CodeView counterpart' in e for e in errors))

    def test_unlisted_windows_definition_fails(self):
        errors, _ = compare([definition()], [], {}, {})
        self.assertTrue(any(e.startswith('WIN_ONLY ') for e in errors))

    def test_filter_is_exact_and_stale_entries_fail(self):
        d = definition()
        key = (d.file, d.name, d.signature)
        self.assertEqual(compare([d], [], {}, {key: 'Complete-added method'})[0], [])
        errors, _ = compare([definition(signature='int ()')], [], {},
                            {key: 'Complete-added method'})
        self.assertTrue(any(e.startswith('WIN_ONLY ') for e in errors))
        self.assertTrue(any('stale win_only' in e for e in errors))

    def test_filters_require_reasons_and_reject_duplicates(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'dc_only.tsv'
            path.write_text('file\tfunction\tline\treason\n'
                            'port.cpp\tport_init\t10\tWinCE-only initialization\n'
                            'port.cpp\tport_init\t10\t\n')
            _, errors = read_filter(path, ('file', 'function', 'line'))
            self.assertTrue(errors)


class DefinitionScannerTest(unittest.TestCase):
    def test_conversion_operators_preserve_member_constness(self):
        from dataclasses import replace
        from homm3.match.source_ownership import scan_unit
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'src').mkdir()
            (root / 'include').mkdir()
            (root / 'src/value.cpp').write_text(
                'struct Value { operator bool() const { return true; } '
                'operator int() { return 1; } };\n')
            definitions, errors, _ = scan_unit({'source': 'src/value.cpp'}, root)
            self.assertEqual(errors, [])
            bodies = {d.name: d for d in definitions}
            boolean = bodies['Value::operator bool']
            self.assertTrue(boolean.const)
            self.assertFalse(bodies['Value::operator int'].const)
            dc = replace(origin(name=boolean.name, file='value.cpp'),
                         argument_types=(), const=True)
            self.assertEqual(compare([boolean], [dc], {}, {})[0], [])
            errors, _ = compare([replace(boolean, const=False)], [dc], {}, {})
            self.assertTrue(any(e.startswith('SIGNATURE ') for e in errors))

    def test_macro_annotated_inline_and_union_bodies_use_ast_flags(self):
        from homm3.match.source_ownership import scan_unit
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'src').mkdir()
            (root / 'include').mkdir()
            (root / 'include/va.h').write_text(
                '#define VA(a,b) __attribute__((annotate("va:" #a " size:" #b)))\n')
            (root / 'src/inline.cpp').write_text(
                '#include "va.h"\n'
                'struct Widget { int get() const; };\n'
                'VA(0x00401000, 3) inline int Widget::get() const { return 1; }\n'
                'union Value { int n; VA(0x00402000, 3) int get() { return n; } };\n'
                '__forceinline int forced() { return 1; }\n'
                'int ordinary() { return 1; }\n')
            definitions, errors, _ = scan_unit({'source': 'src/inline.cpp'}, root)
            self.assertEqual(errors, [])
            bodies = {d.name: d for d in definitions}
            self.assertTrue(all(bodies[n].inline for n in
                                ('Widget::get', 'Value::get', 'forced')))
            self.assertIsNotNone(bodies['Value::get'].class_offset)
            self.assertFalse(bodies['ordinary'].inline)

    def test_active_header_bodies_constructors_and_unannotated_templates(self):
        from homm3.match.source_ownership import scan_unit
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'src').mkdir()
            (root / 'include').mkdir()
            (root / 'include/widgets.h').write_text(
                '#define VA(a,b) __attribute__((annotate("va:" #a " size:" #b)))\n'
                'struct Widget { int value; Widget(int x) : value(x) {}\n'
                'VA(0x00401000, 3) int get() const { return value; }\n'
                'void declaration(); };\n'
                'template<class T> inline T helper(T x) { return x; }\n')
            (root / 'src/widgets.cpp').write_text(
                '// Unicode evidence comment: café — source.\n#include "widgets.h"\n'
                '#if 0\nvoid inactive() {}\n#endif\n'
                'void Widget::declaration() { legacy_body_name_not_parsed; }\n')
            definitions, errors, _ = scan_unit({'source': 'src/widgets.cpp'}, root)
            self.assertEqual(errors, [])
            self.assertEqual({d.name for d in definitions},
                             {'Widget::Widget', 'Widget::get', 'Widget::declaration', 'helper'})
            getter = next(d for d in definitions if d.name == 'Widget::get')
            self.assertEqual(getter.file, 'include/widgets.h')
            self.assertEqual(getter.va, 0x401000)
            self.assertTrue(getter.inline)
            method = next(d for d in definitions if d.name == 'Widget::declaration')
            raw = (root / method.file).read_text()
            self.assertEqual(raw[method.offset:method.end],
                             'void Widget::declaration() { legacy_body_name_not_parsed; }')

    def test_header_va_order_is_not_retail_emission_order(self):
        from homm3.match.verify_va_claims import check
        claims = [(1, 'VA', 0x402000, 8), (2, 'VA', 0x401000, 8)]
        functions = {0x1000: 8, 0x2000: 8}
        self.assertEqual(check({'include/a.h': claims}, functions, {}), [])
        self.assertTrue(any(v[0] == 'ORDER' for v in
                            check({'src/a.cpp': claims}, functions, {})))

    def test_dc_size_in_a_claim_comment_is_not_its_identity(self):
        from homm3.match.source_ownership import origin_hint
        raw = '// E:\\gamedcs\\window.cpp:200\nVA(0x401000, 10) // size 2x dc 0x1e0, dc 0x14111c\nvoid f() {}'
        self.assertEqual(origin_hint(raw, raw.index('f()')),
                         ('window.cpp', 200, '0x14111c'))

class LocalClassTest(unittest.TestCase):
    def test_enum_admission_is_limited_to_proven_class_member_scope(self):
        from homm3.cleanliness import board
        source = '''class LocalDialog {
    enum { OK = 500 };
    void f() { enum { LOCAL = 501 }; }
    class Unproven { enum { NESTED = 502 }; };
};
enum Outside { OTHER = 503 };
'''
        self.assertEqual(len(board._cpp_local_enum_sites(source, {})), 4)
        sites = board._cpp_local_enum_sites(source, {'dc_local_classes': {'LocalDialog'}})
        self.assertEqual(len(sites), 3)
        self.assertTrue(all(source.index('enum { OK') not in range(i, i + 12) for i in sites))

    def test_only_unique_source_proven_local_class_is_admitted(self):
        from unittest import mock
        from homm3.cleanliness import board
        from homm3.match.source_ownership import Origin
        path = Path('/tmp/example/src/dialog.cpp')
        source = 'class LocalDialog { int value; };'
        dc = [Origin('dialog.cpp', 'LocalDialog::LocalDialog', 10, 1, 'dialog.obj', '0x1000')]
        with mock.patch('homm3.match.source_ownership.read_dc', return_value=dc):
            self.assertEqual(board._dc_local_classes([(path, source)])[path], {'LocalDialog'})
            self.assertEqual(board._dc_local_classes([(path, source),
                             (Path('/tmp/example/include/dialog.h'), source)])[path], set())
        dc.append(Origin('dialog.h', 'LocalDialog::get', 20, 1, 'other.obj', '0x2000'))
        with mock.patch('homm3.match.source_ownership.read_dc', return_value=dc):
            self.assertEqual(board._dc_local_classes([(path, source)])[path], set())
        self.assertTrue(board._cpp_local_view_sites(source, {}))

class CoverageTest(unittest.TestCase):
    def test_reference_carcass_cannot_hide_implementation(self):
        from homm3.match.source_ownership import reference_stubs_only
        stub = '#include <va.h>\nDC_ONLY(0x1000, 8)\nvoid Reference() { /* @stub */ }\n'
        self.assertTrue(reference_stubs_only(stub))
        self.assertFalse(reference_stubs_only(stub.replace('/* @stub */', 'work();')))
        self.assertFalse(reference_stubs_only(stub + 'void hidden() {}\n'))
        self.assertFalse(reference_stubs_only('#define HIDDEN void hidden() {}\n'))

    def test_doubled_windows_path_separators(self):
        from homm3.match.source_ownership import source_file
        self.assertEqual(source_file(r'E:\\gamedcs\\philai.cpp'), 'philai.cpp')

    def test_formal_types_disambiguate_const_overload(self):
        from dataclasses import replace
        d = replace(definition(), const=True)
        origins = [replace(origin(line=100), argument_types=(), const=False),
                   replace(origin(line=110), argument_types=(), const=True)]
        self.assertEqual(compare([d], origins, {}, {})[0], [])

    def test_formal_arity_ignores_optimized_debug_parameter_count(self):
        from dataclasses import replace
        d = replace(definition(), parameters=1, argument_types=('int',))
        origins = [replace(origin(line=100), parameters=0, argument_types=('int',)),
                   replace(origin(line=110), parameters=2, argument_types=('int', 'int'))]
        self.assertEqual(compare([d], origins, {}, {})[0], [])

    def test_new_constructor_cannot_borrow_another_overloads_owner(self):
        from dataclasses import replace
        d = definition(name='Widget::Widget')
        o = replace(origin(name=d.name), argument_types=('int',))
        errors, counts = compare([d], [o], {}, {})
        self.assertEqual(counts, {'signature': 1})
        self.assertTrue(errors[0].startswith('SIGNATURE '))
        # A reviewed Windows-only overload is still an exact exception.
        key = (d.file, d.name, d.signature)
        self.assertEqual(compare([d], [o], {}, {key: 'New Windows overload'})[0], [])

    def test_platform_signature_change_needs_an_explicit_identity(self):
        from dataclasses import replace
        d = replace(definition(), parameters=2, argument_types=('int', 'int'))
        o = replace(origin(), argument_types=('int',))
        self.assertTrue(compare([d], [o], {}, {})[0][0].startswith('SIGNATURE '))
        # A source annotation names the independently reviewed DC procedure.
        self.assertEqual(compare([replace(d, dc_offset=o.offset)], [o], {}, {})[0], [])

    def test_constness_mismatch_does_not_silently_inherit_source_order(self):
        from dataclasses import replace
        d = definition()
        o = replace(origin(), argument_types=(), const=True)
        errors, counts = compare([d], [o], {}, {})
        self.assertEqual(counts, {'signature': 1})
        self.assertTrue(errors[0].startswith('SIGNATURE '))


class LinkOrderOriginTest(unittest.TestCase):
    def test_unannotated_body_consumes_its_own_source_origin(self):
        from homm3.analysis.link_order import parse_unit
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'dialog.cpp'
            path.write_text('// E:\\gamedcs\\dialog.cpp:10\n'
                            'int helper() { return 1; }\n'
                            'VA(0x401000, 8)\nvoid unrelated() {}\n'
                            '// E:\\gamedcs\\dialog.cpp:20\n'
                            'VA(0x402000, 8)\nvoid owned() {}\n')
            self.assertEqual(parse_unit(path), [('dialog.cpp', 20, 'VA', 0x402000, 8)])


class HeaderClaimOwnershipTest(unittest.TestCase):
    def test_inactive_cpp_claim_does_not_own_header_body(self):
        from dataclasses import replace
        from types import SimpleNamespace
        from homm3.match.source_ownership import header_claim_ownership
        d = replace(definition(), mangled='?draw@Widget@@QAEXXZ')
        claim = SimpleNamespace(kind='func', channel='src-VA+base',
                                name=d.mangled, rva=0x1000)
        self.assertTrue(header_claim_ownership([d], [claim]))
        self.assertEqual(header_claim_ownership([replace(d, va=0x401000)], [claim]), [])
        self.assertEqual(header_claim_ownership([d], []), [])


class InlineCppOrderTest(unittest.TestCase):
    def test_inline_claim_does_not_advance_ordinary_rva_order(self):
        from homm3.match.verify_va_claims import check
        rows = {'src/lobby.cpp': [(1, 'VA', 0x401000, 8),
                                 (2, 'VA', 0x409000, 8),
                                 (3, 'VA', 0x402000, 8)]}
        functions = {0x1000: 8, 0x2000: 8, 0x9000: 8}
        self.assertTrue(check(rows, functions, {}))
        self.assertEqual(check(rows, functions, {}, {('src/lobby.cpp', 0x409000)}), [])
        rows['src/lobby.cpp'][-1] = (3, 'VA', 0x400800, 8)
        functions[0x800] = 8
        self.assertTrue(check(rows, functions, {}, {('src/lobby.cpp', 0x409000)}))


class CompilerIdentityTest(unittest.TestCase):
    def test_generated_enrollment_without_a_vc6_public_is_fatal(self):
        from homm3.retail_labels import Claim
        from homm3.match.source_ownership import compgen_identity
        raw = '__h3cg$window$class_ctor$Reply'
        claim = Claim(0x1000, raw, 'func', 'src-VA_COMPGEN', 33, 'window',
                      dict(ckind='CLASS_CTOR', owner='Reply', raw=raw))
        self.assertTrue(compgen_identity([claim])[0].startswith('CLAIM_COMPGEN '))
        paired = claim._replace(name='??0Reply@@QAE@XZ', channel='src-VA+base')
        self.assertEqual(compgen_identity([paired]), [])

    def test_anonymous_generated_thunk_keeps_its_deliberate_name(self):
        from homm3.retail_labels import Claim
        from homm3.match.source_ownership import compgen_identity
        claim = Claim(0x1000, '__h3cg$window$static_ctor$record', 'func',
                      'src-VA_COMPGEN', 33, 'window',
                      dict(ckind='STATIC_CTOR', owner='record'))
        self.assertEqual(compgen_identity([claim]), [])

    def test_retained_address_with_raw_name_is_not_a_valid_identity(self):
        from dataclasses import replace
        from types import SimpleNamespace
        from homm3.match.source_ownership import claim_identity
        d = replace(definition(file='src/lobby.cpp'), va=0x401000,
                    mangled='??0Reply@@QAE@HH@Z')
        claim = SimpleNamespace(kind='func', rva=0x1000, name='Reply')
        self.assertTrue(claim_identity([d], [claim]))
        claim.name = d.mangled
        self.assertEqual(claim_identity([d], [claim]), [])

    def test_ast_keeps_inline_cpp_annotation_and_source_destructor_name(self):
        from homm3.match.source_ownership import scan_unit
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'src').mkdir()
            (root / 'include').mkdir()
            raw = ('#define VA(a,b) __attribute__((annotate("va:" #a " size:" #b)))\n'
                   'class Reply { public: VA(0x401000, 8) Reply(int x) {}\n'
                   'virtual ~Reply() {} };\n')
            (root / 'src/lobby.cpp').write_text(raw)
            definitions, errors, _ = scan_unit({'source': 'src/lobby.cpp'}, root)
            self.assertEqual(errors, [])
            ctor = next(d for d in definitions if d.name == 'Reply::Reply')
            dtor = next(d for d in definitions if d.name == 'Reply::~Reply')
            self.assertEqual(ctor.va, 0x401000)
            self.assertEqual(ctor.class_offset, raw.index('class Reply'))
            self.assertEqual(dtor.mangled, '??1Reply@@UAE@XZ')


class NestedLocalClassTest(unittest.TestCase):
    def test_nested_source_class_still_requires_unique_physical_definition(self):
        from unittest import mock
        from homm3.cleanliness import board
        path = Path('/tmp/example/src/dialog.cpp')
        source = 'class Outer { class Saved { int value; }; };'
        dc = [Origin('dialog.cpp', 'Outer::Outer', 10, 1, 'dialog.obj', '0x1000'),
              Origin('dialog.cpp', 'Outer::Saved::Saved', 11, 1, 'dialog.obj', '0x2000')]
        with mock.patch('homm3.match.source_ownership.read_dc', return_value=dc):
            allowed = board._dc_local_classes([(path, source)])[path]
            self.assertEqual(allowed, {'Outer', 'Saved'})
            duplicate = (Path('/tmp/example/include/duplicate.h'), 'class Saved { int other; };')
            self.assertNotIn('Saved', board._dc_local_classes([(path, source), duplicate])[path])


if __name__ == '__main__':
    unittest.main()
