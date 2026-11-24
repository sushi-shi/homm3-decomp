"""The RTTI audit reads retail locators and object ??_R3 chains alike and
reports hierarchy, scope, consistency and catchable-type differences."""
import struct
import tempfile
import unittest
from pathlib import Path

from homm3.core.pe import Pe
from homm3.delink.coffx import Obj
from homm3.verify import rtti

BASE = 0x400000
TEXT, RDATA, DATA = 0x1000, 0x2000, 0x3000
TYPE_INFO = BASE + RDATA + 0x3F0          # the shared type_info vftable
ANON = ".?AVHidden@?%C:\\Dev\\Game\\Foo Bar.cpp123456@@"


class _Image:
    """A three-section PE: code, .rdata records, .data type descriptors."""

    def __init__(self):
        self.rdata, self.data = bytearray(), bytearray()

    def _put(self, buf, rva, payload):
        while len(buf) % 4:
            buf.append(0)
        at = BASE + rva + len(buf)
        buf += payload
        return at

    def td(self, name):
        return self._put(self.data, DATA, struct.pack("<II", TYPE_INFO, 0)
                         + name.encode("latin-1") + b"\0")

    def words(self, *values):
        return self._put(self.rdata, RDATA, struct.pack(f"<{len(values)}I", *values))

    def hierarchy(self, attributes, bases):
        """bases: [(td, contained, mdisp, pdisp, vdisp, attributes)]."""
        bcds = [self.words(td, c, m & 0xFFFFFFFF, p & 0xFFFFFFFF, v, a)
                for td, c, m, p, v, a in bases]
        array = self.words(*bcds)
        chd = self.words(0, attributes, len(bcds), array)
        self.words(0, 0, 0, bases[0][0], chd)            # its locator
        return chd

    def throw(self, types):
        """types: [(properties, td, mdisp, pdisp, vdisp, size, copy)]."""
        cts = [self.words(p, td, m, q & 0xFFFFFFFF, v, s, c)
               for p, td, m, q, v, s, c in types]
        cta = self.words(len(cts), *cts)
        return self.words(0, BASE + TEXT + 0x10, 0, cta)

    def pe(self, directory):
        data = bytearray(0x400)
        data[:2] = b"MZ"
        struct.pack_into("<I", data, 0x3C, 0x80)
        data[0x80:0x84] = b"PE\0\0"
        struct.pack_into("<HH", data, 0x84, 0x14C, 3)
        struct.pack_into("<H", data, 0x94, 0xE0)
        struct.pack_into("<H", data, 0x98, 0x10B)
        struct.pack_into("<I", data, 0x98 + 28, BASE)
        raw = [(".text", TEXT, bytes(0x200)), (".rdata", RDATA, bytes(self.rdata)),
               (".data", DATA, bytes(self.data))]
        pointer = 0x400
        for i, (name, rva, payload) in enumerate(raw):
            size = -(-max(len(payload), 1) // 0x200) * 0x200
            header = 0x178 + 40 * i
            data[header:header + 8] = name.encode().ljust(8, b"\0")
            struct.pack_into("<4I", data, header + 8, size, rva, size, pointer)
            pointer += size
        for _name, _rva, payload in raw:
            data += payload.ljust(-(-max(len(payload), 1) // 0x200) * 0x200, b"\0")
        path = Path(directory) / "retail.exe"
        path.write_bytes(bytes(data))
        return Pe(path)


def _object(records):
    """An i386 COFF with one section holding `records` = [(symbol, [word |
    relocation target name])]; targets with no record stay undefined."""
    payload, relocs, offsets = bytearray(), [], {}
    for name, words in records:
        offsets[name] = len(payload)
        for word in words:
            if isinstance(word, str):
                relocs.append((len(payload), word))
                word = 0
            payload += struct.pack("<I", word & 0xFFFFFFFF)
    names = list(offsets) + sorted({t for _o, t in relocs} - set(offsets))
    index = {name: i for i, name in enumerate(names)}
    header_size = 20 + 40
    rel_at = header_size + len(payload)
    sym_at = rel_at + 10 * len(relocs)
    out = bytearray(struct.pack("<HHIIIHH", 0x14C, 1, 0, sym_at, len(names), 0, 0))
    out += b".rdata\0\0" + struct.pack("<IIIIIIHHI", 0, 0, len(payload), header_size,
                                         rel_at, 0, len(relocs), 0, 0x40000040)
    out += payload
    for site, target in relocs:
        out += struct.pack("<IIH", site, index[target], 6)
    strings = bytearray(4)
    for name in names:
        out += struct.pack("<II", 0, len(strings))
        out += struct.pack("<IhHBB", offsets.get(name, 0), 1 if name in offsets else 0,
                           0, 2, 0)
        strings += name.encode("latin-1") + b"\0"
    struct.pack_into("<I", strings, 0, len(strings))
    return Obj(bytes(out + strings))


def _r0(td):
    return "??_R0" + td[1:] + "@8"


def _class_records(td, attributes, bases, tag=""):
    """??_R3/??_R2/??_R1/??_R0 records of one class; bases as _Image."""
    cls = td[4:-2]
    records, r1s = [], []
    for i, (name, c, m, p, v, a) in enumerate(bases):
        r1 = f"??_R1{tag}{i}{cls}"
        r1s.append(r1)
        records.append((r1, [_r0(name), c, m, p, v, a]))
        records.append((_r0(name), [0]))
    records.append((f"??_R2{cls}@@8", r1s))
    records.append((f"??_R3{cls}@@8", [0, attributes, len(bases), f"??_R2{cls}@@8"]))
    return list({n: (n, w) for n, w in records}.values())


class RetailTest(unittest.TestCase):
    def test_locators_and_throw_records(self):
        image = _Image()
        base, derived, hidden = image.td(".?AVBase@@"), image.td(".?AVDerived@@"), image.td(ANON)
        image.hierarchy(0, [(derived, 1, 0, -1, 0, 0), (base, 0, 0, -1, 0, 9)])
        image.hierarchy(0, [(hidden, 0, 0, -1, 0, 0)])
        image.throw([(0, derived, 0, -1, 0, 8, 0), (0, base, 0, -1, 0, 4, 0)])
        with tempfile.TemporaryDirectory() as directory:
            r = rtti.Retail(image.pe(directory))
            found, spelled, type_info = rtti.retail_hierarchies(r)
            thrown = rtti.retail_catchables(r, type_info)
        self.assertEqual(type_info, TYPE_INFO)
        self.assertEqual(found[".?AVDerived@@"], rtti.Hierarchy(0, (
            rtti.Base(".?AVDerived@@", 1, 0, -1, 0, 0), rtti.Base(".?AVBase@@", 0, 0, -1, 0, 9))))
        self.assertIn(".?AVHidden@?%foobar.cpp@@", found)
        self.assertEqual(spelled[".?AVHidden@?%foobar.cpp@@"], ANON)
        self.assertEqual([c.name for c in thrown[".?AVDerived@@"]],
                         [".?AVDerived@@", ".?AVBase@@"])


class ObjectTest(unittest.TestCase):
    def test_reviewed_scope_and_hierarchy(self):
        ours = ".?AVHidden@?%Z:\\checkout\\src\\foo_bar.cpp987@@"
        records = _class_records(ours, 0, [(ours, 0, 0, -1, 0, 0)])
        canonical = {("foo", "foo_bar.cpp"): "C:\\Dev\\Game\\Foo Bar.cpp123456"}
        found = rtti.our_records([("foo", _object(records))], canonical)
        key = ".?AVHidden@?%foobar.cpp@@"
        self.assertEqual(list(found.hierarchies), [key])
        self.assertEqual(list(found.spelled[key]), [ANON])

    def test_catchable_records(self):
        records = [("__CTA1?AVErr@@", [1, "__CT??_R0?AVErr@@@84"]),
                   ("__CT??_R0?AVErr@@@84",
                    [0, "??_R0?AVErr@@@8", 0, -1, 0, 12, "??0Err@@QAE@ABV0@@Z"])]
        found = rtti.our_records([("unit", _object(records))], {}, eh=True)
        self.assertEqual(found.catchables[".?AVErr@@"],
                         {(rtti.Catchable(".?AVErr@@", 0, 0, -1, 0, 12, True),): ["unit"]})


def _h(*bases, attributes=0):
    return rtti.Hierarchy(attributes, tuple(rtti.Base(*b) for b in bases))


def _ours(hierarchies=None, catchables=None):
    return rtti.Ours(hierarchies or {}, {}, catchables or {})


class CompareTest(unittest.TestCase):
    DERIVED = (".?AVDerived@@", 1, 0, -1, 0, 0)

    def test_private_base_attributes_differ(self):
        retail = {".?AVDerived@@": _h(self.DERIVED, (".?AVBase@@", 0, 0, -1, 0, 9))}
        ours = _ours({".?AVDerived@@": {_h(self.DERIVED, (".?AVBase@@", 0, 0, -1, 0, 0)): ["u"]}})
        result = rtti.compare(retail, {}, ours)
        [finding] = result["findings"]
        self.assertEqual(finding["kind"], "hierarchy")
        self.assertIn("Base: attributes retail 0x9", finding["differences"][0])
        self.assertEqual(result["summary"]["agree"], 0)

    def test_base_order_and_agreement(self):
        a, b = (".?AVA@@", 0, 0, -1, 0, 0), (".?AVB@@", 0, 4, -1, 0, 0)
        top = (".?AVC@@", 2, 0, -1, 0, 0)
        retail = {".?AVC@@": _h(top, a, b, attributes=1), ".?AVA@@": _h(a)}
        ours = _ours({".?AVC@@": {_h(top, b, a, attributes=1): ["u"]}, ".?AVA@@": {_h(a): ["u"]}})
        result = rtti.compare(retail, {}, ours)
        self.assertEqual(result["summary"]["agree"], 1)
        self.assertTrue(result["findings"][0]["differences"][0].startswith("base order"))

    def test_scope_only_match(self):
        anon = ".?AVTraits@?%terrain.cpp@@"
        retail = {anon: _h((anon, 0, 0, -1, 0, 0))}
        ours = _ours({".?AVTraits@@": {_h((".?AVTraits@@", 0, 0, -1, 0, 0)): ["rmg"]}})
        result = rtti.compare(retail, {}, ours)
        [finding] = result["findings"]
        self.assertEqual((finding["kind"], finding["ours_class"]), ("scope", ".?AVTraits@@"))
        self.assertEqual(len(finding["differences"]), 1)      # nothing else differs
        self.assertEqual((result["retail_only"], result["ours_only"]), ([], []))

    def test_unreviewed_scope_is_a_note(self):
        key = ".?AVT@?%foo.cpp@@"
        h = _h((key, 0, 0, -1, 0, 0))
        ours = rtti.Ours({key: {h: ["foo"]}}, {key: {".?AVT@?%Z:\\src\\foo.cpp9@@": ["foo"]}}, {})
        result = rtti.compare({key: h}, {key: ".?AVT@?%C:\\Dev\\Foo.cpp1@@"}, ours)
        self.assertEqual(result["findings"], [])
        self.assertEqual([n["kind"] for n in result["notes"]], ["unreviewed-scope"])

    def test_inconsistent_objects_and_one_sided(self):
        one, two = _h((".?AVX@@", 0, 0, -1, 0, 0)), _h((".?AVX@@", 0, 0, -1, 0, 8))
        ours = _ours({".?AVX@@": {one: ["a"], two: ["b"]}})
        result = rtti.compare({".?AVY@@": _h((".?AVY@@", 0, 0, -1, 0, 0))}, {}, ours)
        self.assertEqual([f["kind"] for f in result["findings"]], ["inconsistent"])
        self.assertEqual((result["retail_only"], result["ours_only"]),
                         ([".?AVY@@"], [".?AVX@@"]))

    def test_catchable_difference(self):
        err = rtti.Catchable(".?AVErr@@", 0, 0, -1, 0, 12, True)
        retail_ct = {".?AVErr@@": (err,)}
        ours = _ours(catchables={".?AVErr@@": {(err._replace(copy=False),): ["u"]}})
        result = rtti.compare({}, {}, ours, retail_ct)
        [finding] = result["findings"]
        self.assertEqual(finding["kind"], "catchable")
        self.assertEqual(finding["differences"], ["Err: copy retail True, ours False"])

    def test_scope_spellings(self):
        self.assertEqual(rtti.fold("?%C:\\Dev\\Editor\\Map Editor.cpp284242958@"),
                         "?%mapeditor.cpp@")
        self.assertEqual(rtti.strip_anonymous(".?AVT@?%x.cpp@@"), ".?AVT@@")


if __name__ == "__main__":
    unittest.main()
