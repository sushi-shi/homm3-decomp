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

    def test_two_argument_less_template_bridge_checks_both_arguments(self):
        clang = '??$?MII@@YI_NABV?$Point@I@@0@Z'
        vc6 = '??M@YI_NABV?$Point@I@@0@Z'
        self.assertEqual(vc6_function_name(clang, [vc6], 'example'), vc6)
        self.assertIsNone(vc6_function_name(clang.replace('?MII@@', '?MIH@@'), [vc6], 'example'))
        self.assertIsNone(vc6_function_name(clang.replace('?MII@@', '?MIII@@'), [vc6], 'example'))

    def test_member_template_constructor_bridge_drops_argument_list(self):
        clang = '??$?0H@?$Point@I@@QAE@ABV?$Point@H@@@Z'
        vc6 = '??0?$Point@I@@QAE@ABV?$Point@H@@@Z'
        self.assertEqual(vc6_function_name(clang, [vc6], 'example'), vc6)
        self.assertIsNone(vc6_function_name(clang.replace('?0H@', '?0E@'), [vc6], 'example'))
        self.assertIsNone(vc6_function_name(clang, [vc6.replace('QAE', 'QBE')], 'example'))

    def test_member_operator_template_bridge_reads_its_reference_parameter(self):
        # RawStream.h's TRawOStream::operator<< <long> (h3maped 0x40771d)
        clang = '??$?6J@TRawOStream@@QAEAAV0@ABJ@Z'
        vc6 = '??6TRawOStream@@QAEAAV0@ABJ@Z'
        self.assertEqual(vc6_function_name(clang, [vc6], 'example'), vc6)
        # the int instance is another name; an argument the parameter does
        # not carry is no instance of it
        self.assertIsNone(vc6_function_name(clang, [vc6.replace('ABJ', 'ABH')], 'example'))
        self.assertIsNone(vc6_function_name(clang.replace('?6J@', '?6H@'), [vc6], 'example'))
        self.assertIsNone(vc6_function_name(clang, [vc6.replace('QAE', 'QBE')], 'example'))

    def test_free_function_template_bridge_drops_argument_list(self):
        # RawStream.h's writeContainer<TRumor> (h3maped 0x434342)
        clang = ('??$writeContainer@VTRumor@@@@YAXAAVTRawOStream@@'
                 'ABV?$vector@VTRumor@@V?$allocator@VTRumor@@@std@@@std@@@Z')
        vc6 = ('?writeContainer@@YAXAAVTRawOStream@@'
               'ABV?$vector@VTRumor@@V?$allocator@VTRumor@@@std@@@std@@@Z')
        self.assertEqual(vc6_function_name(clang, [vc6], 'example'), vc6)
        # an argument the signature does not carry is not this instance
        self.assertIsNone(vc6_function_name(
            clang.replace('VTRumor@@@@', 'VTEvent@@@@'), [vc6], 'example'))
        self.assertIsNone(vc6_function_name(clang, [vc6.replace('YAX', 'YAH')], 'example'))

    def test_free_function_template_bridge_needs_arguments_the_signature_carries(self):
        # A primitive, pointer or non-type argument cannot be confirmed in
        # the signature: any instance would bridge to the one plain name.
        for arguments in ('H', 'W4Kind@@H', 'PAVTRumor@@', '$0BA@', 'VTRumor@@H'):
            with self.subTest(arguments=arguments):
                clang = f'??$f@{arguments}@@YAXAAVTRumor@@W4Kind@@@Z'
                self.assertIsNone(vc6_function_name(
                    clang, ['?f@@YAXAAVTRumor@@W4Kind@@@Z'], 'example'))
        # class and enum arguments the signature carries still bridge
        vc6 = '?f@@YAXAAUTRumor@@W4Kind@ns@@@Z'
        self.assertEqual(vc6_function_name('??$f@VTRumor@@W4Kind@ns@@@@YAXAAUTRumor@@W4Kind@ns@@@Z',
                                           [vc6], 'example'), vc6)
        self.assertIsNone(vc6_function_name('??$f@VTRumor@@W4Other@@@@YAXAAUTRumor@@W4Kind@ns@@@Z',
                                            [vc6], 'example'))

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
