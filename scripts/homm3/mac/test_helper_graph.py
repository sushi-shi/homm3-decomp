from pathlib import Path
from types import SimpleNamespace
import unittest
import tempfile
import csv
from unittest.mock import patch

from homm3.mac import helper_graph
from homm3.mac.relocations import Address


def node(name, offset=None):
    return {'id': name, 'name': name, 'mac': [(offset, 0x20)] if offset else [],
            'windows': [], 'definition': True}


def edge(caller, callee, offset):
    return {'caller': caller, 'callee': callee, 'dispatch': 'direct',
            'kind': 'call', 'location': {'file': 'src/test.cpp', 'line': 1, 'offset': offset},
            'expression': callee + '()'}


class HelperGraphTests(unittest.TestCase):
    def report(self, edges, branches, code=b'', observations=None):
        source = {'nodes': {'f': node('f', 0x100), 'g': node('g', 0x200), 'wrapper': node('wrapper')},
                  'edges': edges, 'units': ['test'], 'diagnostics': {'test': []}, 'gaps': []}
        claims = [SimpleNamespace(offset=o, path='src/test.cpp', line=1, windows_va=o + 0x400000,
                                  label=n, identity=n) for n, o in [('f', 0x100), ('g', 0x200)]]
        index = SimpleNamespace(branches=[(Address(0, at), Address(0, target), kind) for at, target, kind in branches],
                                indirect_branches=[Address(0, 0)] if code else [],
                                pef=SimpleNamespace(data=b'fixture', sections=[SimpleNamespace(index=0, kind=0)], instantiated=1, contents=lambda _: code))
        with patch.object(helper_graph.tables, 'read_functions', return_value={0x100: 0x20, 0x200: 0x20}), \
             patch.object(helper_graph.target_observations, 'read', return_value=observations or {}), \
             patch.object(helper_graph.tables, 'read_runtime', return_value=[]), \
             patch.object(helper_graph.addresses, 'scan', return_value=(claims, [], [])):
            return helper_graph.build(Path('.'), index, source, complete_source_scope=False)

    def test_observed_operation_does_not_invent_source_identity_or_close_call(self):
        observation = {'category': 'platform', 'operation': 'closeFile',
                       'evidence': 'Calls the imported file-close routine.'}
        report = self.report([], [(0x104, 0x300, 'linked_branch')],
                             observations={0x300: observation})
        row = report['queue'][0]
        self.assertEqual(row['callee']['observation'], observation)
        self.assertEqual(row['callee']['source_ids'], [])
        self.assertIsNone(row['callee']['name'])
        self.assertEqual(row['state'], 'source_callee_unavailable')
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'helper-audit.json'
            helper_graph.write_queues(report, output)
            with output.with_name('helper-audit-calls.tsv').open() as stream:
                saved = next(csv.DictReader(stream, delimiter='\t'))
            self.assertEqual(saved['callee_operation'], 'closeFile')
            self.assertEqual(saved['state'], 'source_callee_unavailable')

    def test_nested_wrapper_is_review_lead_not_closure(self):
        report = self.report([edge('f', 'wrapper', 1), edge('wrapper', 'g', 2)], [(0x104, 0x200, 'linked_branch')])
        row = report['queue'][0]
        self.assertEqual(row['state'], 'inlining_path_requires_review')
        self.assertEqual(len(row['source_paths'][0]), 2)
        self.assertFalse(report['source_scope_complete'])

    def test_repeated_target_counts_do_not_hide_missing_use(self):
        report = self.report([edge('f', 'g', 1)], [(0x104, 0x200, 'linked_branch'), (0x108, 0x200, 'linked_branch')])
        self.assertEqual({r['state'] for r in report['queue']}, {'call_count_difference'})
        self.assertEqual(report['queue'][0]['mac_call_count'], 2)
        self.assertEqual(report['queue'][0]['source_call_count'], 1)

    def test_source_only_use_requires_mac_inline_evidence(self):
        report = self.report([edge('f', 'g', 1)], [])
        self.assertEqual(report['source_queue'][0]['state'], 'mac_inlining_requires_review')

    def test_unpaired_callers_and_tail_references_survive(self):
        report = self.report([], [(0x300, 0x200, 'linked_branch'), (0x104, 0x200, 'branch')])
        refs = report['references']['mac:0:0x200']
        self.assertEqual(len(refs), 2)
        self.assertIsNone(refs[0]['caller'])
        self.assertEqual(refs[1]['kind'], 'branch')
        self.assertEqual([r['state'] for r in report['queue']], ['tail_transfer_requires_review'])

    def test_indirect_link_preserved_without_invented_destination(self):
        # blrl (linked indirect branch), followed by blr (ordinary return).
        report = self.report([], [], bytes.fromhex('4e8000214e800020'))
        self.assertEqual(len(report['indirect_calls']), 1)
        self.assertEqual(report['indirect_calls'][0]['site'], 'mac:0:0x0')
        self.assertIsNone(report['indirect_calls'][0]['caller'])
        self.assertEqual(report['indirect_calls'][0]['state'], 'indirect_destination_unresolved')

    def test_generated_queues_preserve_site_identity(self):
        report = self.report([edge('f', 'g', 1)], [(0x104, 0x200, 'linked_branch')])
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'helper-audit-partial.json'
            helper_graph.write_queues(report, output)
            with output.with_name('helper-audit-partial-calls.tsv').open() as stream:
                rows = list(csv.DictReader(stream, delimiter='\t'))
            self.assertEqual(rows[0]['site'], 'mac:0:0x104')
            self.assertEqual(rows[0]['source_count'], '1')
            self.assertFalse(output.with_name('helper-audit-calls.tsv').exists())

    def test_same_name_selector_requires_unique_identity(self):
        report = self.report([], [])
        report['functions']['mac:0:0x200']['name'] = 'f'
        with self.assertRaises(ValueError):
            helper_graph.select(report, 'f')
        self.assertEqual(helper_graph.select(report, '0x400100')['mac'], 'mac:0:0x100')


if __name__ == '__main__':
    unittest.main()
