"""Negative controls for symbol-backed DC_ADDRESS claims and migration coverage."""
from dataclasses import replace
from types import SimpleNamespace
import unittest

from homm3.analysis.dc_claims import claims
from homm3.match.dc_addresses import validate
from homm3.match.source_ownership import Definition


class ClaimsTest(unittest.TestCase):
    def test_pairing_ignores_comments_and_accepts_wrapped_reordered_annotations(self):
        text = ('// VA(0x401111, 4) DC_ADDRESS(0x222, 4)\n'
                'VA(0x401000, 4) MAC_ADDRESS(0x80, 8)\n'
                'DC_ADDRESS(\n 0x100,\n 12)\nvoid first() {}\n'
                'DC_ADDRESS(0x200, 16)\nVA(0x402000, 8)\nvoid second() {}\n'
                'DC_ADDRESS(0x300, 20)\nvoid standalone() {}\n'
                'const char *s = "DC_ADDRESS(0x444, 4)";\n')
        self.assertEqual([(c.offset,c.size,c.va) for c in claims(text)],
                         [(0x100,12,0x401000),(0x200,16,0x402000),(0x300,20,None)])

    def test_repeated_emissions_do_not_borrow_a_previous_functions_va(self):
        text = ('VA(0x401000, 4)\nvoid f() {}\n'
                'DC_ADDRESS(0x100, 8) DC_ADDRESS(0x200, 12)\nvoid g() {}')
        self.assertEqual([(c.offset,c.va) for c in claims(text)], [(0x100,None),(0x200,None)])

    def setUp(self):
        self.d = Definition('src/widget.cpp', 10, 50, 100, 'Widget::draw',
                            'void ()', 0, True, False, None, '', dc_addresses=((0x100,12),))
        self.symbols = SimpleNamespace(procedures={
            0x100: SimpleNamespace(name='Widget::Draw',size=12),
            0x200: SimpleNamespace(name='Widget::Draw',size=16),
            0x300: SimpleNamespace(name='Widget::Close',size=12)})
        self.rows = [dict(status='matched',source_file=self.d.file,source_line=self.d.line,
                         source_name=self.d.name,signature=self.d.signature,dc_offset='0x100')]

    def check(self,d,rows=None,**kwargs):
        kwargs.setdefault('require_complete', True)
        return validate([d], [], self.rows if rows is None else rows, self.symbols, **kwargs)

    def test_exact_symbol_and_size_pass(self):
        self.assertEqual(self.check(self.d), [])

    def test_wrong_size_interior_address_wrong_identity_and_overload_fail(self):
        for addresses in [((0x100,10),), ((0x102,10),), ((0x300,12),), ((0x200,16),)]:
            self.assertTrue(self.check(replace(self.d,dc_addresses=addresses)),addresses)

    def test_missing_annotation_is_a_temporary_hard_error(self):
        d=replace(self.d,dc_addresses=())
        self.assertIn('MISSING',self.check(d)[0])
        self.assertEqual(self.check(d,require_complete=False), [])
        self.assertTrue(self.check(replace(d,dc_addresses=((0x100,1),)),require_complete=False))

    def test_platform_exemption_cannot_claim_a_different_name(self):
        self.assertEqual(self.check(self.d,rows=[]), [])
        self.assertTrue(self.check(replace(self.d,dc_addresses=((0x300,12),)),rows=[]))

    def test_every_retained_emission_needs_a_claim(self):
        rows=self.rows+[dict(self.rows[0],dc_offset='0x200')]
        self.assertTrue(self.check(self.d,rows=rows))
        self.assertEqual(self.check(replace(self.d,dc_addresses=((0x100,12),(0x200,16))),rows=rows), [])

    def test_duplicate_claim_and_duplicate_owner_fail(self):
        self.assertTrue(self.check(replace(self.d,dc_addresses=((0x100,12),(0x100,12)))))
        errors=validate([self.d,replace(self.d,file='src/duplicate.cpp')],[],self.rows,self.symbols)
        self.assertTrue(any('multiple authored owners' in e for e in errors))

    def test_ast_reads_macro_and_rejects_malformed_values(self):
        import tempfile
        from pathlib import Path
        from homm3.match.source_ownership import scan_unit
        from homm3.match.test_source_ownership import parsing_project
        from homm3.match.dc_addresses import validate_source_sites
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);parsing_project(root)
            (root/'src').mkdir();(root/'include').mkdir()
            source=root/'src/widget.cpp'
            macro='#define DC_ADDRESS(o,s) __attribute__((annotate("dc:" #o " size:" #s)))\n'
            source.write_text(macro+'DC_ADDRESS(0x100, 12)\nvoid draw() {}\n')
            definitions,errors,_=scan_unit({'source':'src/widget.cpp'},root)
            self.assertEqual(errors,[])
            self.assertEqual(definitions[0].dc_addresses,((0x100,12),))
            self.assertEqual(definitions[0].dc_offset,'0x100')
            self.assertEqual(validate_source_sites(root,definitions),[])
            source.write_text(macro+'DC_ADDRESS(0x100, 12)\nvoid draw();\n')
            definitions,errors,_=scan_unit({'source':'src/widget.cpp'},root)
            self.assertTrue(validate_source_sites(root,definitions))
            source.write_text(macro+'DC_ADDRESS(wrong, 12)\nvoid draw() {}\n')
            definitions,errors,_=scan_unit({'source':'src/widget.cpp'},root)
            self.assertTrue(any('malformed' in e for e in errors))
            self.assertTrue(validate_source_sites(root,definitions))

    def test_address_annotations_do_not_change_windows_body_hash(self):
        from homm3.match.status import _canonical_definition_text, _after_windows_claim
        from homm3.retail_labels.source import mask_lexical_noise
        from homm3.core.cpp_tokens import fingerprint
        result=[]
        for annotations in ('', ' DC_ADDRESS(0x100, 12)',
                            ' MAC_ADDRESS(0x200, 16)\nDC_ADDRESS(0x100, 12)\nDC_ADDRESS(0x300, 16)'):
            text='VA(0x401000, 4)'+annotations+'\nint helper() { return 1; }\n'
            masked=mask_lexical_noise(text)
            after=_after_windows_claim(masked,text.index(')'))
            result.append(fingerprint(_canonical_definition_text(text,masked,after,'?helper@@YAHXZ')))
        self.assertEqual(len(set(result)),1)

    def test_mac_and_windows_scanners_skip_dc_annotations(self):
        from homm3.mac.addresses import scan_text
        for text in ('DC_ADDRESS(0x100,12)\nVA(0x401000,4) MAC_ADDRESS(0x200,16)\nvoid helper() {}\n',
                     'VA(0x401000,4) MAC_ADDRESS(0x200,16)\nDC_ADDRESS(0x100,12)\nvoid helper() {}\n'):
            mac,win,errors=scan_text(text,'src/widget.cpp')
            self.assertEqual(errors,[])
            self.assertEqual(win[0].label,'helper')
            self.assertEqual(mac[0].windows_va,0x401000)

    def test_browser_uses_debug_module_for_moved_source_and_header_claims(self):
        import tempfile
        from pathlib import Path
        from homm3.analysis.dreamcast import _source_claims
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'src').mkdir();(root/'include').mkdir()
            (root/'src/moved.cpp').write_text(
                'DC_ADDRESS(0x100,12)\nVA(0x401000,4)\nvoid draw() {}\n')
            (root/'include/widget.h').write_text(
                'DC_ADDRESS(0x200,8)\nVA(0x402000,4)\ninline int width() { return 1; }\n')
            found=_source_claims(root/'src',functions=[
                dict(offset='0x100',module='original.obj'),
                dict(offset='0x200',module='caller.obj')])
            self.assertEqual({(c.va,c.module,c.dc_offset) for c in found},
                             {(0x401000,'original.obj',0x100),(0x402000,'caller.obj',0x200)})

    def test_mac_hash_ignores_dc_annotation_with_character_offsets(self):
        import tempfile
        from pathlib import Path
        from homm3.mac.pairs import _fingerprint
        from homm3.core.cpp_tokens import fingerprint
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'src').mkdir()
            prefix='// résumé\n'
            body='int draw() { return 1; }'
            text=prefix+'DC_ADDRESS(0x100,12)\nMAC_ADDRESS(0x200,16)\n'+body
            (root/self.d.file).write_text(text)
            d=replace(self.d,offset=len(prefix),end=len(text))
            self.assertEqual(_fingerprint(root,d),fingerprint(body))

    def test_wrapping_preserves_original_name_and_other_platform_labels(self):
        import tempfile
        from pathlib import Path
        from homm3.match.source_ownership import original_name_hint
        from homm3.mac.addresses import scan_text
        from homm3.retail_labels.source import scan_file
        text=('// Original: Widget::Draw; widget.cpp:20.\n'
              'VA(0x401000,4) MAC_ADDRESS(0x200,16)\n'
              'DC_ADDRESS(\n    0x100,\n    12)\n'
              'void Widget::draw() {}\n')
        self.assertEqual(original_name_hint(text,text.index('void')),'Widget::Draw')
        mac,win,errors=scan_text(text,'src/widget.cpp')
        self.assertEqual(errors,[])
        self.assertEqual(win[0].label,'Widget::draw')
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'widget.cpp';p.write_text(text)
            rows=scan_file(p,{0x1000})
            self.assertEqual(rows[0]['name'],'Widget_draw')
