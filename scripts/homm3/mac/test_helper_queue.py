"""Source-call leads must distinguish definitions and repeated call sites."""
from pathlib import Path
from types import SimpleNamespace
import tempfile
import unittest
from unittest.mock import patch

from homm3.mac import addresses, helper_queue, tables
from homm3.mac.relocations import Address


class TestHelperQueue(unittest.TestCase):
    def report(self, authored, *, sites=1, error=None):
        caller = SimpleNamespace(retail_va=0x400100, unit='caller', signature='int Derived::save',
                                 source=Path('caller.cpp'), mac_section=0, mac_offset=0x100,
                                 mac_size=0x40, source_helper=None)
        callee = SimpleNamespace(retail_va=None, unit='base', signature='int Base::save',
                                 source_helper='Base::save', mac_section=0, mac_offset=0x200)
        index = SimpleNamespace(pef=None, branches=[
            (Address(0, 0x100 + 4 * i), Address(0, 0x200), 'linked_branch')
            for i in range(sites)])
        inventory = {'rows': [{'retail_va': '0x00400100', 'unit': 'caller',
                               'function': 'Derived::save', 'windows_max': 100}]}
        with tempfile.TemporaryDirectory() as directory, \
                patch.object(helper_queue.references, 'load', return_value=[callee]), \
                patch.object(helper_queue, 'load_pairs', return_value=[caller]), \
                patch.object(helper_queue, 'extract_body', return_value=authored, side_effect=error), \
                patch.object(helper_queue.glue, 'imports', return_value={}), \
                patch.object(tables, 'read_runtime', return_value=[]), \
                patch.object(addresses, 'legacy_runtime', return_value=[]):
            return helper_queue.generate(Path(directory), inventory, index, all_functions=True)

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
