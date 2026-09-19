"""Nominal template identity is preserved in generated claims and ABI joins."""
import unittest

from homm3.retail_labels.source import _demangle_key, vc6_function_name


class TemplatePointNamesTest(unittest.TestCase):
    def test_free_less_template_bridge_preserves_complete_abi(self):
        clang = '??$?MI@@YI_NABU?$Point@I@@0@Z'
        vc6 = '??M@YI_NABU?$Point@I@@0@Z'
        self.assertEqual(vc6_function_name(clang, [vc6], 'example'), vc6)
        for wrong in (vc6.replace('YI_N', 'YA_N'), vc6.replace('ABU', 'PBU'),
                      vc6.replace('Point@I', 'Other@I'), vc6.replace('Point@I', 'Point@H')):
            self.assertIsNone(vc6_function_name(clang, [wrong], 'example'))
        self.assertIsNone(vc6_function_name(clang.replace('?MI@@', '?MH@@'), [vc6], 'example'))

    def test_unsigned_and_signed_template_tree_owners_stay_distinct(self):
        for code, owner in [('I', 'point_unsigned_int'), ('H', 'point_int')]:
            tree = f'?$_Tree@U?$Point@{code}@@U1@U_Kfn@'
            for function, key in [('?insert@', 'tree_insert'), ('?find@', 'tree_find'),
                                  ('?_Lbound@', 'tree_lbound'), ('?_Ubound@', 'tree_ubound'),
                                  ('?_Dec@const_iterator@', 'tree_const_iterator_dec')]:
                self.assertEqual(_demangle_key(function + tree), owner + '@' + key)
            self.assertEqual(_demangle_key('?_Distance@std@@YIX' + tree), owner + '@std_distance')
            self.assertEqual(_demangle_key('?_Distance@std@@YIX' + tree + 'Ubidirectional_iterator_tag@'),
                             owner + '@std_distance_tagged')

    def test_unrecognized_template_arguments_do_not_alias_unsigned_owner(self):
        for argument in ('PAX', 'HI', 'I@foreign@', '$0BA@'):
            result = _demangle_key('?insert@?$_Tree@U?$Point@' + argument + '@@U1@U_Kfn@')
            self.assertNotEqual(result, 'point_unsigned_int@tree_insert')


if __name__ == '__main__':
    unittest.main()
