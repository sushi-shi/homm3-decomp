"""The link-diff gate states post-link edits and ratchets per region."""
import struct
import tempfile
import unittest
from pathlib import Path

from homm3.verify import link_diff


def _edits(rows: str) -> link_diff.Edits:
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "edits.tsv"
        path.write_text("kind\toffset\tsize\tretail\tlink\twhat\tevidence\n" + rows)
        return link_diff.read_edits(path)


def _image() -> bytes:
    data = bytearray(0x400)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HH", data, 0x84, 0x14C, 1)
    struct.pack_into("<H", data, 0x94, 0xE0)
    struct.pack_into("<I", data, 0x98 + 28, 0x400000)
    struct.pack_into("<4I", data, 0x178 + 8, 0x100, 0x1000, 0x200, 0x200)
    data[0x178:0x17D] = b".text"
    return bytes(data)


class LinkDiffTest(unittest.TestCase):
    def test_edits_revert_to_link_bytes_and_check_retail(self):
        edits = _edits("bytes\t0x10\t2\tabcd\t0000\twhat\tevidence\n")
        image = bytearray(_image())
        image[0x10:0x12] = b"\xab\xcd"
        view, findings = link_diff.link_view(bytes(image), edits)
        self.assertEqual(findings, [])
        self.assertEqual(view[0x10:0x12], b"\0\0")
        image[0x10] = 0
        _view, findings = link_diff.link_view(bytes(image), edits)
        self.assertEqual(len(findings), 1)

    def test_edit_sizes_must_agree(self):
        with self.assertRaisesRegex(ValueError, "disagrees"):
            _edits("bytes\t0x10\t3\tabcd\t0000\twhat\tevidence\n")

    def test_import_rules(self):
        edits = _edits("import-dll-name\tKERNEL32.dll\t-\tKeRNeL32.dll\tKERNEL32.dll\tw\te\n"
                       "import-hints\tUSER32.dll\t-\t0\tlibrary\tw\te\n"
                       "import-timestamp\t*\t-\t0xad2b0000\t0x0\tw\te\n")
        self.assertEqual(edits.dll_names, {"kernel32.dll": "KeRNeL32.dll"})
        self.assertEqual(edits.zero_hints, {"user32.dll"})
        self.assertEqual(edits.timestamp, 0xAD2B0000)

    def test_ratchet(self):
        counts = {name: 1 for name in link_diff.GATED + link_diff.TRACKED}
        report = link_diff.Report(counts, {})
        self.assertEqual(link_diff.gate_findings(report, dict(counts)), [])
        rising = dict(counts, imports=2)
        self.assertEqual(link_diff.gate_findings(link_diff.Report(rising, {}), counts),
                         ["imports: 2 > ceiling 1"])
        missing = {k: v for k, v in counts.items() if k != "rsrc"}
        self.assertEqual(link_diff.gate_findings(report, missing),
                         ["rsrc: 1, no banked ceiling"])

    def test_rich_difference(self):
        def rich(entries):
            key = 0x1234
            body = struct.pack("<4I", 0x536E6144 ^ key, key, key, key)
            for comp, count in entries:
                body += struct.pack("<II", comp ^ key, count ^ key)
            return b"\0" * 0x80 + body + b"Rich" + struct.pack("<I", key)
        total, rows = link_diff.rich_difference(rich([(1, 3), (2, 1)]), rich([(1, 2)]))
        self.assertEqual(total, 2)
        self.assertEqual(len(rows), 2)


if __name__ == "__main__":
    unittest.main()
