"""Negative controls for complete, bidirectional source identity coverage."""
from dataclasses import replace
import tempfile
import unittest
from pathlib import Path

from homm3.match.source_inventory import reconcile, validate_dc_roster
from homm3.match.source_ownership import Definition, Origin, original_name_hint, scan_unit
from homm3.match.test_source_ownership import parsing_project


def definition(name='Widget::draw'):
    return Definition('src/widget.cpp', 20, 20, 30, name, 'void ()', 0,
                      True, False, 0x401000, '?draw', argument_types=(), return_type='void')


def origin(name='Widget::Draw'):
    return Origin('widget.cpp', name, 100, 1, 'widget.obj', '0x1000', (), return_type='void')


class SourceInventoryTest(unittest.TestCase):
    def test_module_filter_cannot_hide_global_collection_failure(self):
        from types import SimpleNamespace
        from unittest.mock import patch
        from homm3.match import source_inventory as inventory
        dc = origin()
        symbols = SimpleNamespace(procedures={0x1000:
            SimpleNamespace(name=dc.name, module=dc.module)})
        disposition = {(dc.file, dc.name, str(dc.line)): 'Reviewed removed interface.'}
        with tempfile.TemporaryDirectory() as temp, \
                patch.object(inventory, 'Project'), \
                patch.object(inventory.ownership, 'collect',
                             return_value=([], ['CLANG unrelated TU scan failed'], {})), \
                patch.object(inventory.inputs, 'dreamcast_symbols', return_value=symbols), \
                patch.object(inventory.ownership, 'read_filter',
                             side_effect=[(disposition, []), ({}, [])]):
            result = inventory.audit(Path(temp), modules=['widget'], origins=[dc])
        self.assertEqual(result['unresolved'], 0)
        self.assertEqual(result['violations'], ['CLANG unrelated TU scan failed'])
        self.assertFalse(result['complete'])

    def test_raw_procedures_cannot_disappear_from_generated_roster(self):
        from types import SimpleNamespace
        symbols = SimpleNamespace(procedures={0x1000:
            SimpleNamespace(name='Widget::Draw', module='widget.obj')})
        self.assertEqual(validate_dc_roster([origin()], symbols), [])
        self.assertTrue(validate_dc_roster([], symbols))
        self.assertTrue(validate_dc_roster([origin(), origin()], symbols))
        self.assertTrue(validate_dc_roster([replace(origin(), name='Widget::Erase')], symbols))
        self.assertTrue(validate_dc_roster([replace(origin(), module='other.obj')], symbols))

    def test_deleted_dc_function_is_not_a_clean_empty_inventory(self):
        rows, errors = reconcile([], [origin()], {}, {})
        self.assertEqual(errors, [])
        self.assertEqual(rows[0]['status'], 'missing_source')
        self.assertEqual(rows[0]['dc_name'], 'Widget::Draw')

    def test_valid_name_normalization_matches(self):
        rows, errors = reconcile([definition()], [origin()], {}, {})
        self.assertEqual(errors, [])
        self.assertEqual([r['status'] for r in rows], ['matched'])

    def test_same_named_helpers_are_bound_to_each_source_owner(self):
        first = replace(definition('rgbToHSV'), file='src/bitmap24.cpp', member=False)
        second = replace(first, file='src/palette.cpp')
        first_dc = replace(origin('RGBToHSV'), file='bitmap24.cpp', module='bitmap24.obj')
        second_dc = replace(first_dc, file='palette.cpp', module='palette.obj', offset='0x2000')
        rows, errors = reconcile([first, second], [first_dc, second_dc], {}, {})
        self.assertEqual(errors, [])
        self.assertEqual([(r['dc_file'], r['source_file'], r['status']) for r in rows],
                         [('bitmap24.cpp', 'src/bitmap24.cpp', 'matched'),
                          ('palette.cpp', 'src/palette.cpp', 'matched')])
        rows, errors = reconcile([first], [first_dc, second_dc], {}, {})
        self.assertEqual(errors, [])
        self.assertEqual([r['status'] for r in rows], ['matched', 'missing_source'])
        # A same-file duplicate remains an error even when another TU has
        # an identically named helper; the other owner must not mask it.
        _, errors = reconcile([first, replace(first, offset=40), second],
                              [first_dc, second_dc], {}, {})
        self.assertTrue(any(e.startswith('DUPLICATE ') for e in errors))

    def test_wrong_offset_cannot_rename_a_different_method(self):
        dc = [origin(), replace(origin('Widget::Erase'), offset='0x2000', line=110)]
        rows, errors = reconcile([replace(definition(), dc_offset='0x2000')], dc, {}, {})
        self.assertTrue(any(e.startswith('IDENTITY ') for e in errors))
        self.assertEqual(sum(r['status'] == 'missing_source' for r in rows), 2)
        self.assertTrue(any(r['status'] == 'invalid_source' for r in rows))

    def test_offset_cannot_select_another_same_arity_overload(self):
        d = replace(definition(), dc_offset='0x1000', parameters=1,
                    argument_types=('char *',), signature='void (char *)')
        first = replace(origin(), argument_types=('int',))
        second = replace(first, argument_types=('char *',), offset='0x2000', line=120)
        rows, errors = reconcile([d], [first, second], {}, {})
        self.assertTrue(any('different formal overload' in error for error in errors))
        self.assertFalse(any(row['status'] == 'matched' for row in rows))

    def test_explicit_rename_still_checks_the_actual_original_name(self):
        d = replace(definition('Widget::paint'), original_name='Widget::Draw', dc_offset='0x1000')
        rows, errors = reconcile([d], [origin()], {}, {})
        self.assertEqual(errors, [])
        self.assertEqual(rows[0]['status'], 'matched')
        _, errors = reconcile([replace(d, original_name='Widget::Erase')], [origin()], {}, {})
        self.assertTrue(any(e.startswith('IDENTITY ') for e in errors))

    def test_dc_disposition_needs_an_exact_live_key(self):
        o = origin()
        key = (o.file, o.name, str(o.line))
        rows, errors = reconcile([], [o], {key: 'Removed in Complete; reviewed callers.'}, {})
        self.assertEqual(errors, [])
        self.assertEqual(rows[0]['status'], 'documented_dc_only')
        _, errors = reconcile([], [], {key: 'Old entry'}, {})
        self.assertTrue(any('stale dc_only.tsv' in e for e in errors))

    def test_exempting_both_sides_cannot_hide_a_live_counterpart(self):
        d, o = definition(), origin()
        rows, errors = reconcile([d], [o], {(o.file, o.name, str(o.line)): 'Removed'},
                                 {(d.file, d.name, d.signature): 'Added'})
        self.assertTrue(any('active counterpart' in e for e in errors))

    def test_platform_shim_can_replace_a_same_signature_library_function(self):
        d = definition('libraryCall')
        o = replace(origin('libraryCall'), file='h3/dc_precompiledheaders.cpp')
        rows, errors = reconcile([d], [o], {(o.file, o.name, str(o.line)): 'WinCE shim'},
                                 {(d.file, d.name, d.signature): 'Native Windows implementation'})
        self.assertEqual(errors, [])
        self.assertEqual({r['status'] for r in rows}, {'documented_dc_only', 'documented_win_only'})

    def test_unnamed_namespace_uses_positive_mangled_scope(self):
        d = replace(definition(), mangled='?draw@Widget@?A0x1234@@QAEXXZ', dc_offset='0x1000')
        o = origin("`anonymous namespace'::Widget::Draw")
        rows, errors = reconcile([d], [o], {}, {})
        self.assertEqual(errors, [])
        self.assertEqual(rows[0]['status'], 'matched')
        _, errors = reconcile([replace(d, mangled='?draw@Widget@@QAEXXZ')], [o], {}, {})
        self.assertTrue(any(e.startswith('IDENTITY ') for e in errors))

    def test_unlocated_declaration_cannot_make_an_authored_body_disappear(self):
        d = replace(definition(), file='include/widget.h', inline=True, va=None,
                    class_offset=1, declaration_only_type=0x1234)
        o = replace(origin(), file='', line=0, module='', offset='',
                    declaration_only=True, type_index=0x1234)
        rows, errors = reconcile([d], [o], {}, {})
        self.assertEqual(errors, [])
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]['status'], 'invalid_source')

    def test_windows_only_function_requires_disposition(self):
        d = definition('Widget::newFeature')
        rows, _ = reconcile([d], [], {}, {})
        self.assertEqual(rows[0]['status'], 'missing_dc')
        key = d.file, d.name, d.signature
        rows, errors = reconcile([d], [], {}, {key: 'Complete expansion feature; retail call evidence.'})
        self.assertEqual(errors, [])
        self.assertEqual(rows[0]['status'], 'documented_win_only')
        _, errors = reconcile([], [], {}, {key: 'Stale'})
        self.assertTrue(any('stale win_only.tsv' in e for e in errors))

    def test_repeated_header_emissions_are_all_accounted_for(self):
        d = replace(definition(), file='include/widget.h', inline=True, dc_offset='0x1000')
        o = replace(origin(), file='widget.h')
        repeated = replace(o, module='other.obj', offset='0x2000')
        rows, errors = reconcile([d], [o, repeated], {}, {})
        self.assertEqual(errors, [])
        self.assertEqual([r['status'] for r in rows], ['matched', 'matched'])
        self.assertEqual({r['module'] for r in rows}, {'widget', 'other'})

    def test_same_arity_overloads_keep_distinct_identities(self):
        d = replace(definition(), parameters=1, argument_types=('int',), signature='void (int)')
        o = replace(origin(), argument_types=('int',))
        other = replace(o, line=110, offset='0x2000', argument_types=('char *',))
        rows, errors = reconcile([d], [o, other], {}, {})
        self.assertEqual(errors, [])
        self.assertEqual([r['status'] for r in rows], ['matched', 'missing_source'])

    def test_one_generic_body_covers_its_concrete_emissions(self):
        d = replace(definition('Box::draw'), template=True, dc_offset='0x1000')
        first = replace(origin('Box<int>::Draw'), line=100)
        second = replace(first, name='Box<char>::Draw', module='other.obj', offset='0x2000')
        rows, errors = reconcile([d], [first, second], {}, {})
        self.assertEqual(errors, [])
        self.assertEqual([r['status'] for r in rows], ['matched', 'matched'])

    def test_reviewed_interface_cannot_borrow_another_overload(self):
        d = replace(definition(), parameters=1, signature='void (char *)',
                    argument_types=('char *',), dc_offset='0x1000')
        old = replace(origin(), argument_types=('int', 'int'))
        sibling = replace(origin(), argument_types=('int',), line=120, offset='0x2000')
        rows, errors = reconcile([d], [old, sibling],
            {(old.file, old.name, str(old.line)): 'Old interface changed in Complete'},
            {(d.file, d.name, d.signature): 'Retail uses a char pointer'})
        self.assertEqual(errors, [])
        self.assertEqual({r['status'] for r in rows},
                         {'documented_dc_only', 'documented_win_only', 'missing_source'})

    def test_declared_body_gap_requires_an_explicit_table_disposition(self):
        d = replace(definition(), file='include/widget.h', inline=True, va=None,
                    class_offset=1, declaration_only_type=0x1234)
        o = replace(origin(), file='', line=0, module='', offset='',
                    declaration_only=True, type_index=0x1234)
        rows, errors = reconcile([d], [o], {},
            {(d.file, d.name, d.signature): 'Exact DC declaration; body provenance unknown'})
        self.assertEqual(errors, [])
        self.assertEqual(rows[0]['status'], 'documented_win_only')
        # The table cannot waive the exact declaration proof.
        _, errors = reconcile([replace(d, declaration_only_type=0x9999)], [o], {},
            {(d.file, d.name, d.signature): 'Unproven type'})
        self.assertTrue(any(e.startswith('DECLARATION_ONLY ') for e in errors))

    def test_compiler_and_library_bodies_do_not_silently_disappear(self):
        rows, errors = reconcile([], [replace(origin(), generated=True),
                                      replace(origin('std::helper'), file='../stlport/helper.h',
                                              offset='0x2000')], {}, {})
        self.assertEqual(errors, [])
        self.assertTrue(all(r['status'] == 'missing_source' for r in rows))

    def test_duplicate_source_bodies_are_not_complete(self):
        d = definition()
        _, errors = reconcile([d, replace(d, line=40, offset=40)], [origin()], {}, {})
        self.assertTrue(any(e.startswith('DUPLICATE ') for e in errors))

    def test_original_name_is_attached_and_scanned(self):
        raw = '// Original: Widget::Draw; widget.cpp:100, dc 0x1000.\nvoid paint() {}\nvoid next() {}\n'
        self.assertEqual(original_name_hint(raw, raw.index('void paint')), 'Widget::Draw')
        self.assertEqual(original_name_hint(raw, raw.index('void next')), '')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parsing_project(root)
            (root / 'src').mkdir()
            (root / 'src/widget.cpp').write_text(raw)
            definitions, errors, _ = scan_unit({'source': 'src/widget.cpp'}, root)
            self.assertEqual(errors, [])
            self.assertEqual(definitions[0].original_name, 'Widget::Draw')


if __name__ == '__main__':
    unittest.main()
