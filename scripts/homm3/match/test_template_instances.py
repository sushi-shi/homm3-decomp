"""Typed generic claims select their actual source body, without emission."""
import tempfile
import unittest
from pathlib import Path

from homm3.match.source_ownership import scan_unit


class TemplateInstanceTest(unittest.TestCase):
    def scan(self, body):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'src').mkdir()
            (root / 'include').mkdir()
            (root / 'include/point.h').write_text(
                '#define VA(a,b) __attribute__((annotate("va:" #a " size:" #b)))\n'
                + body)
            (root / 'src/point.cpp').write_text('#include "point.h"\n')
            return scan_unit({'source': 'src/point.cpp'}, root)

    def test_overloaded_constructor_instances_keep_one_generic_body_each(self):
        definitions, errors, _ = self.scan(
            'struct Signed { int x; };\n'
            'template<class T> struct Point { T x,y;\n'
            '// VA instance: Point<unsigned int>::Point(const unsigned int&, const unsigned int&)\n'
            'VA(0x00401000,24) Point(const T& a,const T& b):x(a),y(b){}\n'
            'Point(const Signed& p); };\n'
            'template<class T>\n'
            '// VA instance: Point<unsigned int>::Point(const Signed&)\n'
            'VA(0x00402000,22) Point<T>::Point(const Signed& p):x(p.x),y(p.x){}\n')
        self.assertEqual(errors, [])
        self.assertEqual([d.mangled for d in definitions], [
            '??0?$Point@I@@QAE@ABI0@Z', '??0?$Point@I@@QAE@ABUSigned@@@Z'])
        self.assertTrue(all(d.file == 'include/point.h' for d in definitions))

    def test_constructor_cannot_claim_other_overload(self):
        _, errors, _ = self.scan(
            'template<class T> struct Point {\n'
            '// VA instance: Point<unsigned int>::Point(int)\n'
            'VA(0x00401000,24) Point(const T&, const T&){}\n'
            'Point(int){} };\n')
        self.assertTrue(any('does not name the annotated definition' in e for e in errors), errors)

    def test_constructor_requires_exact_types_not_implicit_conversions(self):
        for parameters in ('const int&, const int&', 'unsigned int, unsigned int',
                           'unsigned int&, unsigned int&'):
            with self.subTest(parameters=parameters):
                _, errors, _ = self.scan(
                    'template<class T> struct Point {\n'
                    f'// VA instance: Point<unsigned int>::Point({parameters})\n'
                    'VA(0x00401000,24) Point(const T&, const T&){} };\n')
                self.assertTrue(any('constructor parameter types do not match' in e for e in errors), errors)

    def test_constructor_accepts_canonical_type_aliases(self):
        definitions, errors, _ = self.scan(
            'typedef unsigned int Coordinate;\n'
            'template<class T> struct Point {\n'
            '// VA instance: Point<Coordinate>::Point(const Coordinate&, const unsigned&)\n'
            'VA(0x00401000,24) Point(const T&, const T&){} };\n')
        self.assertEqual(errors, [])
        self.assertEqual(definitions[0].mangled, '??0?$Point@I@@QAE@ABI0@Z')

    def test_constructor_cannot_claim_explicit_specialization(self):
        _, errors, _ = self.scan(
            'template<class T> struct Point {\n'
            '// VA instance: Point<unsigned int>::Point(const unsigned int&)\n'
            'VA(0x00401000,24) Point(const T&){} };\n'
            'template<> Point<unsigned int>::Point(const unsigned int&){}\n')
        self.assertTrue(any('does not name the annotated definition' in e for e in errors), errors)

    def test_constructor_rejects_wrong_name_and_malformed_types(self):
        for selector in ('Point<unsigned int>::Other(int)',
                         'Point<unsigned int>::Point(int())',
                         'Point<unsigned int>::Point(const int&, )'):
            with self.subTest(selector=selector):
                _, errors, _ = self.scan(
                    'template<class T> struct Point {\n'
                    f'// VA instance: {selector}\n'
                    'VA(0x00401000,24) Point(const T&){} };\n')
                self.assertTrue(any(e.startswith('INSTANCE ') for e in errors), errors)

    def test_free_function_and_operator_template_instances(self):
        definitions, errors, _ = self.scan(
            'template<class T> struct Point { T x; };\n'
            'template<class T>\n'
            '// VA instance: operator< <unsigned int>\n'
            'VA(0x00401000,32) bool operator<(const Point<T>& a,const Point<T>& b){return a.x<b.x;}\n'
            'template<class T>\n'
            '// VA instance: identity<unsigned int>\n'
            'VA(0x00402000,3) T identity(T x){return x;}\n')
        self.assertEqual(errors, [])
        claims = [d for d in definitions if d.va]
        self.assertEqual(claims[0].mangled, '??$?MI@@YI_NABU?$Point@I@@0@Z')
        self.assertEqual(claims[1].mangled, '??$identity@I@@YIII@Z')

    def test_free_template_cannot_borrow_other_body_or_specialization(self):
        for selector, extra in (
            ('other<unsigned int>', 'template<class T> T other(T x){return x;}\n'),
            ('identity<unsigned int>', 'template<> unsigned int identity<unsigned int>(unsigned int x){return x;}\n')):
            with self.subTest(selector=selector):
                prefix = extra if selector.startswith('other') else ''
                suffix = extra if not prefix else ''
                _, errors, _ = self.scan(prefix + 'template<class T>\n'
                    f'// VA instance: {selector}\n'
                    'VA(0x00401000,3) T identity(T x){return x;}\n' + suffix)
                self.assertTrue(any('does not name the annotated definition' in e for e in errors), errors)


if __name__ == '__main__':
    unittest.main()
