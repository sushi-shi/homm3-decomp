"""PE/resource/EH positive evidence and rejected malformed structures."""
import struct
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.sema.retail_claims import collect, eh_claims, resource_claims
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


def resource_fixture():
    data = fixture()
    struct.pack_into('<I', data, 0x98 + 92, 16)
    struct.pack_into('<II', data, 0x98 + 96 + 16, 0x1000, 0x100)
    struct.pack_into('<HH', data, 0x400 + 12, 0, 2)
    struct.pack_into('<4I', data, 0x410, 1, 0x40, 2, 0x50)
    struct.pack_into('<4I', data, 0x440, 0x2000, 8, 0, 0)
    struct.pack_into('<4I', data, 0x450, 0x2000, 8, 0, 0)
    return data


class EvidenceTest(unittest.TestCase):
    def test_shared_resource_payload_keeps_every_owner(self):
        claims = resource_claims(Layout(resource_fixture()))
        payloads = [c for c in claims if c['kind'] == 'resource-payload']
        self.assertEqual([c['owner'] for c in payloads], ['/1', '/2'])
        self.assertTrue(all(c['start'] == 0x2000 and c['size'] == 8 for c in payloads))

    def test_resource_cycles_metadata_escape_and_payload_escape_rejected(self):
        for kind in ('cycle', 'metadata', 'payload'):
            data = resource_fixture()
            if kind == 'cycle':
                struct.pack_into('<I', data, 0x414, 0x80000000)
            elif kind == 'metadata':
                struct.pack_into('<I', data, 0x414, 0xf8)
            else:
                struct.pack_into('<I', data, 0x440, 0x21fc)
            with self.subTest(kind=kind), self.assertRaises(ValueError):
                resource_claims(Layout(data))

    def test_eh_requires_actual_runtime_jump_and_file_backed_map(self):
        data = fixture()
        # Turn .data into .text for the evidence walker contract.
        data[0x178 + 40:0x178 + 48] = b'.text\0\0\0'
        struct.pack_into('<I', data, 0x408, 0x401040)
        struct.pack_into('<i', data, 0x606, 0x2050 - 0x200a)
        info = dict(info_rva=0x1000, max_state=1, n_try=0)
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'config/retail').mkdir(parents=True)
            (root / 'config/retail/runtime-map.tsv').write_text('rva\tname\n0x2050\t___CxxFrameHandler\n')
            with patch('homm3.vc6.tryblocks.func_infos', return_value=[info]), \
                 patch('homm3.vc6.tryblocks.handler_stubs', return_value={0x1000: 0x2000}):
                claims = eh_claims(Layout(data), SimpleNamespace(), root)
                self.assertEqual({c['kind'] for c in claims}, {'eh-funcinfo', 'eh-handler-stub', 'eh-unwind-map'})
                struct.pack_into('<i', data, 0x606, 0)
                self.assertFalse(eh_claims(Layout(data), SimpleNamespace(), root))
                struct.pack_into('<i', data, 0x606, 0x2050 - 0x200a)
                struct.pack_into('<I', data, 0x408, 0x4011fc)
                with self.assertRaises(ValueError):
                    eh_claims(Layout(data), SimpleNamespace(), root)

    def test_absent_optional_navigation_is_explicit_not_a_lost_byte(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'config/retail').mkdir(parents=True)
            (root / 'config/units.toml').write_text('unit = []\n')
            for name, header in [('functions.tsv', 'rva\tsize'), ('vtables.tsv', 'rva\tfunction_count\tclass'),
                                 ('data-extents.tsv', 'rva\tsize\tcategory\tevidence'), ('runtime-map.tsv', 'rva\tname')]:
                (root / 'config/retail' / name).write_text(header + '\n')
            claims, labels, inputs, limitations = collect(root, Layout(fixture()), SimpleNamespace())
            self.assertTrue(claims)
            self.assertFalse(labels)
            self.assertEqual(len(limitations), 2)
            self.assertTrue(any('symbol_names.csv' in s for s in limitations))
