"""Pairing leads must preserve section coordinates and pointer provenance."""
import struct
import unittest

from homm3.mac.discovery import Index, parse_address
from homm3.mac.pef import PEF
from homm3.mac.relocations import Address
from homm3.mac.test_loader import container


class TestMacDiscovery(unittest.TestCase):
    def index(self):
        data = bytearray(128)
        struct.pack_into(">III", data, 0, 0, 0x40, 0x50)
        data[0x50:0x55] = b"table"
        fixture = container(bytes(data), (0x4600, 0x4200))
        image = bytearray(fixture.data)
        start = fixture.section(0).file_offset
        code = bytes.fromhex("48000021 886300d8 a08300da 8062ffc8 806100d9 4e800020")
        image[start:start + len(code)] = code
        return Index(PEF(bytes(image)))

    def test_direct_calls_and_loader_toc_chain(self):
        index = self.index()
        code = index.xrefs(Address(0, 0x20))
        self.assertEqual(code["code_branches"], [{"section": 0, "offset": 0, "kind": "linked_branch"}])
        data = index.xrefs(Address(1, 0x50))
        self.assertEqual(data["loader_pointers"], [{"section": 1, "offset": 8}])
        self.assertEqual(data["toc_uses"][0]["offset"], 12)
        self.assertTrue(data["toc_uses"][0]["via_pointer"])

    def test_literal_and_member_searches_keep_leads_unadmitted(self):
        index = self.index()
        self.assertEqual(index.find_bytes(b"table"), [Address(1, 0x50)])
        self.assertEqual(index.find_bytes(b"table", section=0), [])
        hits = index.find_fields([0xd8, 0xda], window=16)
        self.assertEqual(len(hits), 1)
        self.assertEqual((hits[0]["start"], hits[0]["end"]), (4, 12))
        self.assertEqual(index.find_fields([0xd8, 0xd9], window=32), [])  # SP is not an object.
        with self.assertRaises(ValueError):
            index.find_bytes(b"")

    def test_address_parser_preserves_section(self):
        self.assertEqual(parse_address("mac:1:0x44bd4"), Address(1, 0x44bd4))
        self.assertEqual(parse_address("mac:0xf6290"), Address(0, 0xf6290))
        for text in ("0x004da3a0", "mac:-1:0", "mac:0:-4", "mac:0:0:0"):
            with self.assertRaises(ValueError):
                parse_address(text)


if __name__ == "__main__":
    unittest.main()
