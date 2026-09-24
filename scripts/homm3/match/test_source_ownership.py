"""Negative controls for definition scanning and retail claim ownership."""
import tempfile
import unittest
from pathlib import Path

from homm3.match.source_ownership import Definition


def parsing_project(root):
    config = root / 'config'
    config.mkdir(exist_ok=True)
    manifest = config / 'units.toml'
    if not manifest.exists():
        manifest.write_text('[build]\nincludes=["include"]\nanalysis_profile="test"\n'
                            '[flags]\ntest=["/Gr", "/GX", "/D_WINDOWS"]\n')


def definition(name='Widget::draw', file='include/widget.h', line=20,
               signature='void ()', inline=True):
    return Definition(file, line, line, line + 1, name, signature, 0, True,
                      inline, None, '')


class OwnershipTest(unittest.TestCase):
    def test_active_stub_gate_uses_active_definitions_and_comment_tokens(self):
        from homm3.match.source_ownership import active_stub_definitions, scan_unit
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parsing_project(root)
            (root / 'src').mkdir()
            (root / 'include').mkdir()
            (root / 'include/widgets.h').write_text(
                'struct Widget { Widget() {} void pending() { /* @stub */ } };\n')
            (root / 'src/widgets.cpp').write_text(
                '// Unicode evidence: café — source.\n#include "widgets.h"\n'
                '#if 0\nvoid inactive() { /* @stub */ }\n#endif\n'
                'void active() {\n    // @stub\n}\n'
                'const char* literal() { return "// @stub /* @stub */"; }\n'
                'const char* escaped() { return "\\\" // @stub"; }\n'
                "char slash() { return '/'; }\n"
                'void empty() {}\n'
                'void historical() { /* Previously an @stub, now a real empty body. */ }\n')
            definitions, errors, _ = scan_unit({'source': 'src/widgets.cpp'}, root)
            self.assertEqual(errors, [])
            errors = active_stub_definitions(definitions, root)
            self.assertEqual(len(errors), 2, errors)
            self.assertTrue(any('include/widgets.h:1 Widget::pending' in error for error in errors))
            self.assertTrue(any('src/widgets.cpp:6 active' in error for error in errors))
            self.assertTrue(all(error.startswith('ACTIVE-STUB ') for error in errors))


class DefinitionScannerTest(unittest.TestCase):
    def scan_instance_fixture(self, body, extra=''):
        from homm3.match.source_ownership import scan_unit
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parsing_project(root)
            (root / 'src').mkdir()
            (root / 'include').mkdir()
            (root / 'include/instance.h').write_text(
                '#define VA(a,b) __attribute__((annotate("va:" #a " size:" #b)))\n'
                + body)
            (root / 'src/instance.cpp').write_text('#include "instance.h"\n' + extra)
            return scan_unit({'source': 'src/instance.cpp'}, root)

    def test_template_instance_selects_concrete_member_without_copied_body(self):
        definitions, errors, _ = self.scan_instance_fixture(
            'template<int N> struct Bits {\n'
            ' // VA instance: Bits<144>::operator*\n'
            ' VA(0x00401000, 3) int operator*() const { return N; }\n'
            '};\n')
        self.assertEqual(errors, [])
        self.assertEqual(len(definitions), 1)
        body = definitions[0]
        self.assertEqual(body.instance, 'Bits<144>::operator*')
        self.assertEqual(body.mangled, '??D?$Bits@$0JA@@@QBEHXZ')
        self.assertEqual(body.file, 'include/instance.h')
        self.assertEqual(body.va, 0x401000)

    def test_out_of_class_template_body_keeps_its_instance_selector(self):
        definitions, errors, _ = self.scan_instance_fixture(
            'template<int N> struct Bits { int operator*() const; };\n'
            'template<int N>\n'
            '// VA instance: Bits<144>::operator*\n'
            'VA(0x00401000, 3) int Bits<N>::operator*() const { return N; }\n')
        self.assertEqual(errors, [])
        self.assertEqual(len(definitions), 1)
        self.assertEqual(definitions[0].mangled, '??D?$Bits@$0JA@@@QBEHXZ')

    def test_multiple_instances_keep_one_body_and_distinct_claim_identities(self):
        from homm3.match.source_ownership import claim_definitions, claim_identity
        from types import SimpleNamespace
        from dataclasses import asdict
        import json
        definitions, errors, _ = self.scan_instance_fixture(
            'template<int N> struct Bits { int operator*() const; };\n'
            'template<int N>\n'
            '// VA instance: Bits<144>::operator*\n'
            'VA(0x00401000, 3)\n'
            '// VA instance: Bits<145>::operator*\n'
            'VA(0x00402000, 4)\n'
            'int Bits<N>::operator*() const { return N; }\n')
        self.assertEqual(errors, [])
        self.assertEqual(len(definitions), 1)
        # JSON caches preserve every instance while ownership counts one body.
        body = Definition(**json.loads(json.dumps(asdict(definitions[0]))))
        claimed = list(claim_definitions([body]))
        self.assertEqual([d.va for d in claimed], [0x401000, 0x402000])
        self.assertEqual([d.mangled for d in claimed],
                         ['??D?$Bits@$0JA@@@QBEHXZ', '??D?$Bits@$0JB@@@QBEHXZ'])
        self.assertEqual(len({(d.file, d.offset, d.end) for d in claimed}), 1)
        claims = [SimpleNamespace(kind='func', rva=d.va - 0x400000, name=d.mangled)
                  for d in claimed]
        self.assertEqual(claim_identity([body], claims), [])
        self.assertTrue(any('0x402000' in e for e in claim_identity([body], claims[:1])))

    def test_secondary_instance_must_name_the_same_physical_body(self):
        _, errors, _ = self.scan_instance_fixture(
            'template<int N> struct Bits {\n'
            ' // VA instance: Bits<144>::get\n'
            ' VA(0x00401000, 3)\n'
            ' // VA instance: Bits<145>::other\n'
            ' VA(0x00402000, 3)\n'
            ' int get() const { return N; } int other() const { return 0; }\n'
            '};\n')
        self.assertTrue(any('does not name the annotated definition' in e for e in errors), errors)

    def test_instance_pairs_cannot_be_duplicated_or_grouped_ambiguously(self):
        for second_va, second_selector, grouped in (
                ('0x00401000', 'Bits<145>::get', False),
                ('0x00402000', 'Bits<144>::get', False),
                ('0x00402000', 'Bits<145>::get', True)):
            with self.subTest(va=second_va, selector=second_selector, grouped=grouped):
                first = '// VA instance: Bits<144>::get\n'
                second = f'// VA instance: {second_selector}\n'
                annotations = ('VA(0x00401000, 3)\n', f'VA({second_va}, 3)\n')
                prefix = (first + second + ''.join(annotations) if grouped else
                          first + annotations[0] + second + annotations[1])
                _, errors, _ = self.scan_instance_fixture(
                    'template<int N> struct Bits {\n' + prefix +
                    'int get() const { return N; } };\n')
                self.assertTrue(any(e.startswith('INSTANCE ') for e in errors), errors)

    def test_template_destructor_instance_uses_ordinary_destructor_name(self):
        definitions, errors, _ = self.scan_instance_fixture(
            'template<class T> struct Owner {\n'
            ' // VA instance: Owner<int>::~Owner\n'
            ' VA(0x00401000, 3) ~Owner() {}\n'
            '};\n')
        self.assertEqual(errors, [])
        self.assertEqual(len(definitions), 1)
        self.assertEqual(definitions[0].mangled, '??1?$Owner@H@@QAE@XZ')

    def test_template_instance_cannot_borrow_another_physical_definition(self):
        for selector, extra in (
                ('Bits<144>::other', ''),
                ('Bits<144>::get', 'template<> int Bits<144>::get() const { return 8; }\n')):
            with self.subTest(selector=selector, specialized=bool(extra)):
                definitions, errors, _ = self.scan_instance_fixture(
                    'template<int N> struct Bits {\n'
                    f' // VA instance: {selector}\n'
                    # Both members share a line: token identity, not merely
                    # line identity, has to distinguish them.
                    ' VA(0x00401000, 3) int get() const { return N; } int other() const { return 0; }\n'
                    '};\n', extra)
                self.assertTrue(any('does not name the annotated definition' in e for e in errors), errors)

    def test_template_instance_requires_one_selector_one_va_and_valid_member(self):
        for hints, annotation in (
                ('// VA instance: Bits<144>::get\n', ''),
                ('// VA instance: Bits<144>::get\n// VA instance: Bits<129>::get\n', 'VA(0x00401000, 3)'),
                ('// VA instance: Bits<144>::missing\n', 'VA(0x00401000, 3)'),
                ('// VA instance: \n', 'VA(0x00401000, 3)')):
            with self.subTest(hints=hints, annotation=annotation):
                _, errors, _ = self.scan_instance_fixture(
                    'template<int N> struct Bits {\n' + hints +
                    annotation + ' int get() const { return N; }\n};\n')
                self.assertTrue(any(e.startswith('INSTANCE ') for e in errors), errors)

    def test_vc6_only_definitions_cannot_hide_from_inventory(self):
        from homm3.match.source_ownership import scan_unit
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parsing_project(root)
            (root / 'src').mkdir()
            (root / 'include').mkdir()
            # Use the real annotation boundary: selecting the VC6 source
            # branch must not erase VA metadata or enable editor bodies.
            actual_root = Path(__file__).resolve().parents[3]
            (root / 'include/va.h').write_text(
                (actual_root / 'include/va.h').read_text())
            (root / 'include/value.h').write_text(
                '#include "va.h"\n'
                '#if defined(_MSC_VER) && !defined(__clang__)\n'
                'struct Value { VA(0x00401000, 3) int get() { return 1; } };\n'
                '#else\n'
                'struct Value { int editorOnly() { return 2; } };\n'
                '#endif\n')
            (root / 'src/value.cpp').write_text(
                '#include "value.h"\n'
                '#if _MSC_VER == 1200 && !defined(__clang__)\n'
                'VA(0x00402000, 3) int retained() { return 1; }\n'
                '#else\n'
                'int editorOnly() { return 2; }\n'
                '#endif\n'
                '#if 0\nint inactive() { return 0; }\n#endif\n')
            definitions, errors, _ = scan_unit({'source': 'src/value.cpp'}, root)
            self.assertEqual(errors, [])
            bodies = {d.name: d for d in definitions}
            self.assertEqual(set(bodies), {'Value::get', 'retained'})
            self.assertEqual(bodies['Value::get'].file, 'include/value.h')
            self.assertEqual(bodies['Value::get'].va, 0x401000)
            self.assertTrue(bodies['Value::get'].inline)
            self.assertEqual(bodies['retained'].va, 0x402000)
            self.assertFalse(bodies['retained'].inline)

    def test_variadic_ast_flag_does_not_count_callback_ellipsis(self):
        from homm3.match.source_ownership import scan_unit
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parsing_project(root)
            (root / 'src').mkdir()
            (root / 'include').mkdir()
            (root / 'src/format.cpp').write_text(
                'int format(const char* pattern, ...) { return 0; }\n'
                'void callback(void (*fn)(const char*, ...)) {}\n'
                'struct Logger { void write(const char* pattern, ...) {} };\n')
            definitions, errors, _ = scan_unit({'source': 'src/format.cpp'}, root)
            self.assertEqual(errors, [])
            bodies = {d.name: d for d in definitions}
            self.assertTrue(bodies['format'].variadic)
            self.assertTrue(bodies['Logger::write'].variadic)
            self.assertFalse(bodies['callback'].variadic)
            self.assertTrue(all(d.parameters == 1 for d in definitions))

    def test_conversion_operators_preserve_member_constness(self):
        from homm3.match.source_ownership import scan_unit
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parsing_project(root)
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

    def test_macro_annotated_inline_and_union_bodies_use_ast_flags(self):
        from homm3.match.source_ownership import scan_unit
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parsing_project(root)
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
            parsing_project(root)
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


class CoverageTest(unittest.TestCase):
    def test_unadmitted_sources_include_empty_and_inactive_files(self):
        from homm3.match.source_ownership import unadmitted_sources
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'src').mkdir()
            (root / 'src/active.cpp').write_text('void active() {}')
            (root / 'src/empty.c').write_text('')
            (root / 'src/reference.cxx').write_text('#if 0\nvoid stub() {}\n#endif\n')
            (root / 'src/notes.md').write_text('reference notes')
            self.assertEqual(unadmitted_sources(root, {'src/active.cpp'}),
                             ['src/empty.c', 'src/reference.cxx'])


class HeaderClaimOwnershipTest(unittest.TestCase):
    def test_removed_claim_cannot_hide_a_banked_header_body(self):
        from dataclasses import replace
        from homm3.match.source_ownership import header_claim_ownership
        from homm3.match.status import MatchRow
        d = replace(definition(), mangled='?draw@Widget@@QAEXXZ')
        banked = {('carrier', d.mangled): MatchRow(None, 100, 100, 0x1000)}
        self.assertTrue(header_claim_ownership([d], [], banked))
        self.assertEqual(header_claim_ownership(
            [replace(d, va=0x401000)], [], banked), [])
        self.assertEqual(header_claim_ownership(
            [replace(d, mangled='?unrelated@Widget@@QAEXXZ')], [], banked), [])

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
    def test_audit_separates_unemitted_generated_body_from_written_identity(self):
        from dataclasses import replace
        from unittest import mock
        from homm3.match.source_ownership import audit
        from homm3.retail_labels import Claim
        raw = '__h3cg$window$class_ctor$Reply'
        claim = Claim(0x1000, raw, 'func', 'src-VA_COMPGEN', 33, 'window',
                      dict(ckind='CLASS_CTOR', owner='Reply', raw=raw))
        body = replace(definition(file='src/window.cpp'), va=0x401000,
                       mangled='??0Reply@@QAE@XZ')
        for definitions in ([], [body]):
            with mock.patch('homm3.match.source_ownership.collect',
                            return_value=(definitions, [], [])), \
                 mock.patch('homm3.match.source_ownership.active_stub_definitions', return_value=[]), \
                 mock.patch('homm3.retail_labels.fragments.all_claims', return_value=[claim]):
                result = audit()
            self.assertEqual(len(result['unpaired_generated_claims']), 1)
            self.assertEqual(bool(result['violations']), bool(definitions))
            if definitions:
                self.assertTrue(result['violations'][0].startswith('CLAIM_IDENTITY '))

    def test_generated_enrollment_without_a_vc6_public_reports_emission_debt(self):
        from homm3.retail_labels import Claim
        from homm3.match.source_ownership import unpaired_generated_claims
        raw = '__h3cg$window$class_ctor$Reply'
        claim = Claim(0x1000, raw, 'func', 'src-VA_COMPGEN', 33, 'window',
                      dict(ckind='CLASS_CTOR', owner='Reply', raw=raw))
        self.assertIn('no VC6 function paired', unpaired_generated_claims([claim])[0])
        paired = claim._replace(name='??0Reply@@QAE@XZ', channel='src-VA+base')
        self.assertEqual(unpaired_generated_claims([paired]), [])

    def test_anonymous_generated_thunk_keeps_its_deliberate_name(self):
        from homm3.retail_labels import Claim
        from homm3.match.source_ownership import unpaired_generated_claims
        claim = Claim(0x1000, '__h3cg$window$static_ctor$record', 'func',
                      'src-VA_COMPGEN', 33, 'window',
                      dict(ckind='STATIC_CTOR', owner='record'))
        self.assertEqual(unpaired_generated_claims([claim]), [])

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
            parsing_project(root)
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


if __name__ == '__main__':
    unittest.main()
