"""Source-call leads must distinguish definitions and repeated call sites."""
from pathlib import Path
from types import SimpleNamespace
import tempfile
import unittest
from unittest.mock import patch

from homm3.mac import addresses, helper_queue, tables
from homm3.mac.relocations import Address


class TestHelperQueue(unittest.TestCase):
    def report(self, authored, *, sites=1, error=None, helper_calls=False, runtime_calls=()):
        caller = SimpleNamespace(retail_va=0x400100, unit='caller', signature='int Derived::save',
                                 source=Path('caller.cpp'), mac_section=0, mac_offset=0x100,
                                 mac_size=0x40, source_helper=None)
        callee = SimpleNamespace(retail_va=None, unit='base', signature='int Base::save',
                                 source_helper='Base::save', mac_section=0, mac_offset=0x200,
                                 mac_size=0x10, source=Path('base.cpp'), identity='source:base:Base::save')
        index = SimpleNamespace(pef=None, branches=[
            (Address(0, 0x100 + 4 * i), Address(0, 0x200), 'linked_branch')
            for i in range(sites)])
        if helper_calls:
            index.branches.append((Address(0, 0x200), Address(0, 0x200), 'linked_branch'))
        for i, label in enumerate(runtime_calls):
            index.branches.append((Address(0, 0x120 + 4 * i), Address(0, label.offset), 'linked_branch'))
        inventory = {'rows': [{'retail_va': '0x00400100', 'unit': 'caller',
                               'function': 'Derived::save', 'windows_max': 100}]}
        with tempfile.TemporaryDirectory() as directory, \
                patch.object(helper_queue.references, 'load', return_value=[callee]), \
                patch.object(helper_queue, 'load_pairs', return_value=[caller]), \
                patch.object(helper_queue, 'extract_body', return_value=authored, side_effect=error), \
                patch.object(helper_queue, '_helper_body', return_value=('int Base::save() { return save(); }' if helper_calls
                                                               else 'int Base::save() { return 0; }')), \
                patch.object(helper_queue.glue, 'imports', return_value={}), \
                patch.object(tables, 'read_runtime', return_value=runtime_calls), \
                patch.object(addresses, 'legacy_runtime', return_value=[]):
            (Path(directory) / "src").mkdir()
            return helper_queue.generate(Path(directory), inventory, index, all_functions=True)

    def test_dispatch_glue_is_an_actionable_unknown_operation(self):
        labels = [tables.RuntimeLabel(0x300, '.__ptr_glue', 'cw_runtime', 'indirect_tvector'),
                  tables.RuntimeLabel(0x400, '.memcpy', 'msl_c', 'direct')]
        report = self.report('int Derived::save() { return Base::save(); }', runtime_calls=labels)
        indirect, direct = report['calls'][1:3]
        self.assertEqual(indirect['state'], 'review_indirect_call')
        self.assertIsNone(indirect['source_call_mentions'])
        self.assertEqual(direct['state'], 'runtime_call')
        self.assertEqual(report['coverage']['indirect_dispatch_calls'], 1)
        self.assertEqual(report['functions'][0]['indirect_dispatch_calls'], 1)
        leads = helper_queue.leads(report)
        self.assertEqual([row['state'] for row in leads], ['review_indirect_call'])
        self.assertEqual(leads[0]['mac_target'], '0:0x300')

    def test_source_helpers_are_callers_without_fabricated_windows_va(self):
        report = self.report('int Derived::save() { return Base::save(); }')
        self.assertEqual(report['coverage']['source_helper_callers'], 1)
        self.assertEqual(report['coverage']['windows_functions_in_scope'], 1)
        helper = report['functions'][1]
        self.assertIsNone(helper['retail_va'])
        self.assertEqual(helper['caller_id'], 'source:base:Base::save')
        self.assertEqual(helper['direct_mac_calls'], 0)

    def test_helper_call_sites_keep_source_identity(self):
        report = self.report('int Derived::save() { return Base::save(); }', helper_calls=True)
        row = report['calls'][1]
        self.assertIsNone(row['retail_va'])
        self.assertEqual(row['caller_id'], 'source:base:Base::save')
        self.assertEqual(row['mac_call_site'], '0:0x200')
        self.assertEqual(row['state'], 'source_call_present')
        self.assertEqual(row['source_call_mentions'], 1)

    def test_new_annotation_supplies_helper_without_toml(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'src/new.cpp'
            source.parent.mkdir()
            source.write_text('MAC_ADDRESS(0x200, 0x4)\nvoid Widget::run() {}\n')
            known = SimpleNamespace(source=source, unit='new', mac_section=0, mac_offset=0x100)
            pef = SimpleNamespace(code=lambda section, offset, size: bytes.fromhex('4e800020'))
            found = helper_queue._source_helpers(root, [known], [], pef)
            self.assertEqual(len(found), 1)
            self.assertEqual(found[0].unit, 'new')
            self.assertEqual(found[0].mac_offset, 0x200)
            self.assertIsNone(found[0].retail_va)
            self.assertIn('Widget::run', helper_queue._helper_body(found[0]))
            source.write_text('MAC_ADDRESS(0x200, 0x4)\nint Widget::run() const { return 1; }\n'
                              'int Widget::run() { return 2; }\n')
            found = helper_queue._source_helpers(root, [known], [], pef)
            self.assertEqual(found[0].source_helper, 'Widget::run() const')
            self.assertIn('return 1;', helper_queue._helper_body(found[0]))
            self.assertNotIn('return 2;', helper_queue._helper_body(found[0]))

    def test_own_definition_does_not_hide_missing_base_call(self):
        report = self.report('int Derived::save() { return 4; }')
        self.assertEqual(report['calls'][0]['source_call_mentions'], 0)
        self.assertEqual(report['calls'][0]['state'], 'review_missing_helper_call')
        fixed = self.report('int Derived::save() { return Base::save(); }')
        self.assertEqual(fixed['calls'][0]['source_call_mentions'], 1)
        self.assertEqual(fixed['calls'][0]['state'], 'source_call_present')

    def test_repeated_and_extra_source_calls_remain_review_leads(self):
        report = self.report('int Derived::save() { return Base::save(); }', sites=2)
        self.assertEqual(report['coverage']['call_count_review_groups'], 1)
        self.assertEqual(report['functions'][0]['direct_mac_calls'], 2)
        self.assertTrue(all(row['state'] == 'review_call_count' for row in report['calls']))
        self.assertEqual(report['calls'][0]['mac_target_calls'], 2)
        self.assertEqual(helper_queue.leads(report)[0]['sites'], 2)
        extra = self.report('int Derived::save() { Base::save(); return Base::save(); }')
        self.assertEqual(extra['calls'][0]['state'], 'review_call_count')

    def test_recursion_is_still_a_call_but_comments_and_literals_are_not(self):
        report = self.report('int Derived::save() { /* save() */ log("save()"); return save(); }')
        self.assertEqual(report['calls'][0]['source_call_mentions'], 1)

    def test_constructor_initializers_survive_but_default_arguments_do_not(self):
        text = helper_queue._call_text('Derived::Derived(int x = factory(1)) : Base(save()) {}')
        self.assertNotIn('factory', text)
        self.assertIn('Base(save())', text)
        with self.assertRaises(ValueError):
            helper_queue._call_text('void broken(')

    def test_unavailable_source_is_not_zero_calls(self):
        report = self.report('', error=ValueError('unresolved compiler-generated claim'))
        row = report['calls'][0]
        self.assertIsNone(row['source_call_mentions'])
        self.assertEqual(row['state'], 'source_unavailable')
        self.assertEqual(report['coverage']['source_unavailable_functions'], 1)
        self.assertTrue(helper_queue.leads(report))


if __name__ == '__main__':
    unittest.main()
