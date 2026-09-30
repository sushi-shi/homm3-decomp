"""Negative controls: a game data byte is exact only when a candidate
definition emits it at that address with the retail value."""
import struct
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
from unittest.mock import patch

from homm3.verify import game_bytes as gb

DATA = 0xC0300040          # initialized data, read/write, 4-byte aligned
BASE = 0x100


def _object(payload, symbols):
    """One ordinary `.data` section defining `symbols` [(name, offset)]."""
    from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
    obj = bytearray(_coff((FixtureSection('.data', payload, ()),),
                          tuple(_symbol(name, offset, 1, 0, 2) for name, offset in symbols)))
    struct.pack_into('<I', obj, 20 + 36, DATA)
    return bytes(obj)


def _compare(payload, symbols, retail, claims):
    """compare_initializers over one candidate object; `claims` [(name, rva, size)]."""
    from homm3.model import Model
    from homm3.verify.byte_accounting import compare_initializers
    image = SimpleNamespace(relocs_in=lambda lo, hi: [])
    pe = SimpleNamespace(image_base=0x400000,
                         read=lambda start, size: retail[start - BASE:start - BASE + size])
    rows = [dict(object='test.c', name=name, rva=hex(rva), size=hex(size), storage='data',
                 section_ordinal='1', section_offset='0x0') for name, rva, size in claims]
    with TemporaryDirectory() as tmp, patch('homm3.delink.image.Image', return_value=image):
        Path(tmp, 'test.obj').write_bytes(_object(payload, symbols))
        return compare_initializers(Model([], [], []), rows, pe, Path(tmp))


def _partition(size, claims):
    from homm3.verify.byte_accounting import Range, partition
    return partition(size, [Range(a, b, category, name, 2) for a, b, category, name in claims])


def _verdicts(comparisons, rows):
    runs = gb.game_runs(rows, text=(0, 0), functions={}, code={},
                        data=gb.data_status(comparisons))
    return gb.apply_runs(rows, runs, default=(gb.GAME_DATA_UNVERIFIED, 'no-comparison'))


def _category_at(rows, address):
    return next(r for r in rows if r['start'] <= address < r['end'])


class LocalStaticAlignmentTests(unittest.TestCase):
    def test_anonymous_element_type_joins_the_emitting_units_layout(self):
        from homm3.core.msvc_names import mask
        from homm3.verify.layout import Layout

        source = '_?levels@?1??initialize@@YIXH@Z@4PAY02VTAutoStrPtr@?A0xB5931CA@@A'
        emitted = r'_?levels@?BC@??initialize@@YIXH@Z@4PAY02VTAutoStrPtr@?%Z:\repo\src\probe.cpp42@@A'
        element = dict(k='rec', sz=4, m=[[0, '.m_ptr', dict(k='ptr', sz=4)]])
        row = dict(k='arr', sz=12, n=3, el=element)
        declaration = dict(t=dict(k='arr', sz=336, n=28, el=row), sz=336)
        layout = Layout(dict(types=[], units={'probe': {'vars': {source: declaration}}}))
        comparison = dict(storage='bss', unit='probe', name=mask(emitted),
                          rva=0x298b9c, size=336, alignment_bound=16,
                          verdict='exact')
        with patch('homm3.model._EMITTED', {'probe': {emitted}}):
            gb.judge_alignment([comparison], layout)
        self.assertEqual((comparison['aligned'], comparison['alignment']), (True, 4))
        self.assertEqual(gb.data_status([comparison])[comparison['name']][0].category,
                         gb.GAME_BSS_EXACT)

        # Equal type layouts do not let another module's anonymous type
        # supply this declaration's identity or prove its alignment.
        comparison.pop('aligned')
        comparison.pop('alignment')
        foreign = emitted.replace('probe.cpp', 'other.cpp')
        comparison['name'] = mask(foreign)
        with patch('homm3.model._EMITTED', {'probe': {foreign}}):
            gb.judge_alignment([comparison], layout)
        self.assertIsNone(comparison['aligned'])

        # A bridge with multiple emitted definitions remains unresolved.
        comparison.pop('aligned')
        comparison['name'] = mask(emitted)
        with patch('homm3.model._EMITTED',
                   {'probe': {emitted, emitted.replace('?BC@??', '?BD@??')}}):
            gb.judge_alignment([comparison], layout)
        self.assertIsNone(comparison['aligned'])


class DefinitionExtentTests(unittest.TestCase):
    def test_undersized_array_is_never_exact_past_its_definition(self):
        # Retail's table has 8 ints; the candidate defines 4, then a neighbour.
        payload = bytes(range(32))
        result, = [r for r in _compare(payload, (('_table', 0), ('_next', 16)),
                                       payload, (('_table', BASE, 32),))]
        self.assertEqual((result['extent'], result['definition'], result['verdict']),
                         ('beyond-candidate-definition', 16, 'mismatch'))
        rows = _verdicts([result], _partition(BASE + 32, [(BASE, BASE + 32, 'game', '_table')]))
        self.assertEqual(_category_at(rows, BASE + 4)['category'], gb.GAME_DATA_EXACT)
        tail = _category_at(rows, BASE + 20)
        self.assertEqual((tail['category'], tail['reason']),
                         (gb.GAME_DATA_UNVERIFIED, 'beyond-candidate-definition'))

    def test_wrong_initializer_byte_is_a_byte_precise_mismatch(self):
        result, = _compare(b'abcd', (('_word', 0),), b'abXd', (('_word', BASE, 4),))
        self.assertEqual(result['verdict'], 'mismatch')
        self.assertEqual(result['wrong'], [[2, 3, '63', '58']])
        rows = _verdicts([result], _partition(BASE + 4, [(BASE, BASE + 4, 'game', '_word')]))
        self.assertEqual(_category_at(rows, BASE + 2)['category'], gb.GAME_DATA_MISMATCH)
        self.assertEqual(_category_at(rows, BASE + 1)['category'], gb.GAME_DATA_EXACT)
        items = gb.build_worklist(rows, SimpleNamespace(read=lambda a, n: b'X', image_base=0x400000),
                                  [result], {BASE: ('_word', 4, 'char[4]')}, {}, None,
                                  SimpleNamespace(functions=[], data=[]))
        mismatch, = [i for i in items if i['category'] == gb.GAME_DATA_MISMATCH]
        self.assertEqual((mismatch['start'], mismatch['size']), (BASE + 2, 1))
        self.assertIn('_word at +0x2: retail 58 candidate 63', mismatch['diagnosis'])

    def test_label_only_extent_is_flagged(self):
        rows = _partition(BASE + 8, [(BASE, BASE + 8, 'game', 'vtbl_100')])
        runs = gb.game_runs(rows, text=(0, 0), functions={}, code={}, data={},
                            labels=frozenset({'vtbl_100'}))
        rows = gb.apply_runs(rows, runs)
        self.assertEqual({(r['category'], r['reason']) for r in rows if r['size'] == 8},
                         {(gb.GAME_DATA_UNVERIFIED, 'label-only-extent')})

    def test_candidate_larger_than_extent_is_not_exact(self):
        self.assertEqual(gb.definition_extent(4, 0, 16, None, False, 4, bytes(4) + b'\1' * 12),
                         ('candidate-larger-than-extent', 16))
        self.assertEqual(gb.definition_extent(6, 0, 8, None, False, 4, b'\xff' * 6 + bytes(2)),
                         ('ok', 6))


class ShortArrayAcceptanceTests(unittest.TestCase):
    """The user's case: retail FF x 12, 00; creatures[6] and heroName[6]."""
    RETAIL = b'\xff' * 12 + b'\0' + b'\x01' * 3

    def claims(self, hero):
        payload = b'\xff' * 6 + (b'\xff' * 6 + b'\0' if hero == 7 else b'\xff' * 6)
        return _compare(payload, (('_creat', 0), ('_hero', 6)), self.RETAIL,
                        (('_creat', BASE, 6), ('_hero', BASE + 6, hero)))

    def rows(self, hero, next_at, padding=False):
        claims = [(BASE, BASE + 6, 'game', '_creat'), (BASE + 6, BASE + 6 + hero, 'game', '_hero'),
                  (BASE + next_at, BASE + next_at + 4, 'game', '_next')]
        if padding:
            claims.append((BASE + 6 + hero, BASE + next_at, 'alignment-padding',
                           'link alignment 4 before 0x110'))
        return _verdicts(self.claims(hero), _partition(BASE + 0x20, claims))

    def test_short_heroname_leaves_the_byte_flagged(self):
        rows = self.rows(6, 0x0D)
        flagged = _category_at(rows, BASE + 0x0C)
        self.assertEqual(flagged['category'], 'missing')
        items = gb.build_worklist(rows, SimpleNamespace(read=lambda a, n: self.RETAIL[a - BASE:a - BASE + n],
                                                        image_base=0x400000),
                                  self.claims(6),
                                  {BASE: ('_creat', 6, 'Creature[6]'),
                                   BASE + 6: ('_hero', 6, 'char[6]')},
                                  {BASE + 6: ('src/x.cpp:123', 'DATA', 'char heroName[6];')},
                                  None, SimpleNamespace(functions=[], data=[]))
        item = next(i for i in items if i['start'] == BASE + 0x0C)
        self.assertEqual(item['previous']['name'], '_hero')
        self.assertEqual(item['next']['name'], '_next')
        self.assertIn('src/x.cpp:123', item['diagnosis'])
        self.assertIn('1 more byte(s) (00)', item['diagnosis'])

    def test_positive_twin_heroname_with_nul_is_exact(self):
        rows = self.rows(7, 0x0D)
        self.assertEqual(_category_at(rows, BASE + 0x0C)['category'], gb.GAME_DATA_EXACT)

    def test_alignment_fill_after_a_fixed_size_datum_is_kept(self):
        proven, _why = gb.end_proven('_count', gb.GAME_DATA_EXACT, 'int')
        self.assertTrue(proven)
        self.assertFalse(gb.end_proven('_name', gb.GAME_DATA_EXACT, 'char[6]')[0])
        self.assertFalse(gb.end_proven('??_C@_03ABCD@abc?$AA@', gb.GAME_DATA_EXACT, '')[0])


if __name__ == '__main__':
    unittest.main()
