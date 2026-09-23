"""Selective analysis-body exclusions must not manufacture data declarations."""
import hashlib
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
import clang.cindex as cx

from homm3.analysis import data_declarations as data, data_body_recovery as recovery


class BodyRecoveryTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root/'src').mkdir()
        self.path = self.root/'src/fixture.cpp'
        self.args = ['--target=i686-pc-windows-msvc', '-fms-extensions', '-std=c++14', '-ferror-limit=0']

    def parse(self, text):
        self.path.write_text(text)
        result = data.parse_unit(dict(root=str(self.root), source='src/fixture.cpp', unit='fixture',
            image_base=0x400000, args=self.args))
        self.assertEqual(self.path.read_text(), text)
        return result

    def test_retains_unaffected_local_extents_and_original_coordinates(self):
        text = '''// UTF-8 source: café
#define DATA(a) __attribute__((annotate("data:" #a)))
void before() { DATA(0x401000) static int values[3]; }
void broken() { static int hidden[7]; missing(); }
void after() { DATA(0x401100) static short rows[2][5]; }
'''
        unit = self.parse(text)
        self.assertEqual(unit['parse_mode'], 'isolated-bodies')
        self.assertFalse(unit['errors'])
        self.assertTrue(unit['full_errors'])
        self.assertTrue(unit['skipped_bodies'])
        facts = {f['name']:f for f in unit['facts']}
        self.assertEqual(set(facts), {'values','rows'})
        self.assertEqual(facts['rows']['shape']['dimensions'], [2,5])
        self.assertEqual(facts['rows']['size'], 20)
        self.assertEqual(facts['rows']['offset'], text.encode().index(b'rows[2]'))
        self.assertNotIn('hidden', {f['name'] for f in unit['definitions']})
        evidence = unit['body_recovery']
        self.assertEqual(evidence['original_sha256'], hashlib.sha256(text.encode()).hexdigest())
        self.assertEqual(len(evidence['regions']), 1)
        region = evidence['regions'][0]
        raw = text.encode()[region['start']:region['end']]
        self.assertTrue(raw.startswith(b'{') and raw.endswith(b'}'))
        self.assertEqual(region['body_sha256'], hashlib.sha256(raw).hexdigest())
        summary = data.summarize([unit], data.annotation_sites(self.root, 0x400000))
        self.assertEqual([r['kind'] for r in summary['issues']], ['bodies-skipped'])
        self.assertFalse(summary['analysis_complete'])

    def test_multiple_diagnostics_exclude_whole_bodies_including_nested_scopes(self):
        unit = self.parse('''
void a() { static int first[2]; { missing(); } }
void b() { { missing_again(); } static int second[3]; }
void valid() { static int kept[4]; }
''')
        self.assertEqual(unit['parse_mode'], 'isolated-bodies')
        self.assertEqual(len(unit['body_recovery']['regions']), 2)
        self.assertEqual([f['name'] for f in unit['definitions']], ['kept'])

    def test_preprocessing_activity_in_excluded_body_keeps_the_full_fallback(self):
        for prefix,body in [('', '#define SIDE_EFFECT 3\nmissing();'),
                            ('', '#if 0\nignored\n#endif\nmissing();'),
                            ('#define CALL missing()\n', 'CALL;'),
                            ('#define COUNT __COUNTER__\n', 'int n=COUNT; missing();'),
                            ('', '%:define SIDE_EFFECT 3\nmissing();'),
                            ('', '__pragma(pack(1)) missing();'),
                            ('', '_Pragma("pack(1)") missing();'),
                            ('', 'int n = 1 + \\\n2; missing();')]:
            with self.subTest(body=body):
                unit=self.parse(prefix+'void bad() {\n'+body+'\n}\nvoid good(){static int kept[4];}\n')
                self.assertEqual(unit['parse_mode'], 'all-bodies-skipped')
                self.assertEqual(unit['body_recovery']['reason'], 'excluded body contains preprocessing activity')
                self.assertFalse(unit['definitions'])

    def test_macro_spelled_body_is_not_a_source_body(self):
        unit=self.parse('#define BODY {missing();}\nvoid bad() BODY\nvoid good(){static int kept[3];}\n')
        self.assertEqual(unit['parse_mode'], 'all-bodies-skipped')
        self.assertEqual(unit['body_recovery']['status'], 'rejected')

    def test_uncontained_signature_global_and_header_errors_are_not_suppressed(self):
        for text in ['void bad(Unknown value) {}\n', 'Unknown global; void bad(){missing();}\n',
                     'struct A { A(): field(missing()) {} int field; };\n']:
            with self.subTest(text=text):
                unit=self.parse(text+'void good(){static int kept[3];}\n')
                self.assertEqual(unit['parse_mode'], 'all-bodies-skipped')
                self.assertEqual(unit['body_recovery']['reason'], 'diagnostic outside an ordinary source body')
        (self.root/'include').mkdir()
        (self.root/'include/error.h').write_text('inline void bad(){missing();}\n')
        unit=self.parse('#include "../include/error.h"\nvoid good(){static int kept[3];}\n')
        self.assertEqual(unit['parse_mode'], 'all-bodies-skipped')
        self.assertEqual(unit['body_recovery']['reason'], 'diagnostic outside an ordinary source body')

    def test_templates_deduced_returns_and_constant_evaluation_are_not_erased(self):
        for text in ['template<class T> void bad(){missing();} template void bad<int>();\n',
                     'template<class T> struct A {void bad(){missing();}}; void call(){A<int> a; a.bad();}\n',
                     'auto bad(){return missing();}\n',
                     '#define RESULT auto\nRESULT bad(){missing();return 3;}\n',
                     'constexpr int bad(){return missing();}\n']:
            with self.subTest(text=text):
                unit=self.parse(text+'void good(){static int kept[3];}\n')
                self.assertEqual(unit['parse_mode'], 'all-bodies-skipped')
                self.assertEqual(unit['body_recovery']['status'], 'rejected')

    def test_function_try_block_is_not_partially_erased(self):
        unit=self.parse('void bad() try {missing();} catch(...) {}\nvoid good(){static int kept[3];}\n')
        self.assertEqual(unit['parse_mode'], 'all-bodies-skipped')

    def test_clean_unit_does_not_get_an_exclusion(self):
        unit=self.parse('void good(){static int kept[3];}\n')
        self.assertEqual(unit['parse_mode'], 'full')
        self.assertFalse(unit['skipped_bodies'])
        self.assertEqual(unit['body_recovery'], {})

    def test_unsupported_encoding_keeps_original_file_and_full_fallback(self):
        raw=b'// non UTF-8: \xff\nvoid bad(){missing();}\nvoid good(){static int kept[3];}\n'
        self.path.write_bytes(raw)
        unit=data.parse_unit(dict(root=str(self.root),source='src/fixture.cpp',unit='fixture',
            image_base=0x400000,args=self.args))
        self.assertEqual(unit['parse_mode'],'all-bodies-skipped')
        self.assertEqual(unit['body_recovery']['reason'],'source encoding is not UTF-8')
        self.assertEqual(self.path.read_bytes(),raw)

    def prepare(self):
        self.path.write_text('void bad(){missing();}\nvoid good(){static int kept[3];}\n')
        index=cx.Index.create();options=cx.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD
        tu=index.parse(str(self.path),args=self.args,options=options)
        return index,tu,options

    def test_reparse_errors_are_retained_and_no_partial_tree_is_admitted(self):
        _,tu,options=self.prepare()
        index=SimpleNamespace(parse=lambda *a,**k:tu)
        parsed,evidence=recovery.recover(index,tu,self.path,self.args,options,
            lambda ty:(data.storage_extent(ty),data.type_shape(ty)))
        self.assertIsNone(parsed)
        self.assertEqual(evidence['reason'],'analysis reparse still has errors')
        self.assertTrue(evidence['errors'])

    def test_changed_retained_declaration_is_not_admitted_even_with_a_clean_parse(self):
        index,tu,options=self.prepare()
        def changed(*args,**kwargs):
            name,text=kwargs['unsaved_files'][0]
            kwargs['unsaved_files']=[(name,text.replace('kept[3]','kept[4]'))]
            return index.parse(*args,**kwargs)
        parsed,evidence=recovery.recover(SimpleNamespace(parse=changed),tu,self.path,self.args,options,
            lambda ty:(data.storage_extent(ty),data.type_shape(ty)))
        self.assertIsNone(parsed)
        self.assertEqual(evidence['reason'],'retained data declarations changed during reparse')


if __name__=='__main__':
    unittest.main()
