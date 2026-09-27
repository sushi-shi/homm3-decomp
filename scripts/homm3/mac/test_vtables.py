"""Reviewed vtables require source declarations and every pinned loader link."""
import hashlib
from pathlib import Path
from types import SimpleNamespace
import tempfile
import unittest

from homm3.mac.object import ObjectError
from homm3.mac.relocations import Address
from homm3.mac.vtables import external_bindings


def digest(data):
    return hashlib.sha256(data).hexdigest()


class Image:
    def __init__(self, code, data):
        self.sections = {0: code, 1: data}

    def section(self, section):
        return SimpleNamespace(kind=0 if section == 0 else 1)

    def read(self, section, offset, size):
        payload = self.sections[section][offset:offset + size]
        if len(payload) != size:
            raise IndexError("outside fixture section")
        return payload


class Loader:
    def __init__(self, pointers):
        self.pointers = pointers

    def toc(self):
        return Address(1, 0x8000)


class TestExternalVtable(unittest.TestCase):
    def fixture(self, root):
        source = root / "include/foo.h"
        source.parent.mkdir(parents=True)
        source.write_text("class Foo : public Base {\n"
                          " virtual int open(int); virtual void close();\n"
                          " virtual int main(int&); };\n")
        code = bytearray(0x40)
        data = bytearray(0x8100)
        data[0x100:0x114] = bytes.fromhex("0000008000000000000002000000021000000220")
        data[0x80:0x8c] = bytes.fromhex("000000700000000000000080")
        slots = []
        pointers = {Address(1, 0x20): Address(1, 0x100),
                    Address(1, 0x100): Address(1, 0x80)}
        for index in range(3):
            descriptor = 0x200 + index * 0x10
            body = 0x10 + index * 0x10
            data[descriptor:descriptor + 8] = body.to_bytes(4, "big") + bytes.fromhex("00008000")
            code[body:body + 4] = bytes((index + 1, 2, 3, 4))
            pointers[Address(1, 0x108 + index * 4)] = Address(1, descriptor)
            pointers[Address(1, descriptor)] = Address(0, body)
            pointers[Address(1, descriptor + 4)] = Address(1, 0x8000)
            slots.append(f'''[[vtables.slots]]
section = 1
offset = {descriptor}
sha256 = "{digest(data[descriptor:descriptor + 8])}"
code_section = 0
code_offset = {body}
code_size = 4
code_sha256 = "{digest(code[body:body + 4])}"
''')
        manifest = root / "config/mac/vtables/foo.toml"
        manifest.parent.mkdir(parents=True)
        manifest.write_text(f'''[[vtables]]
owner_vas = [0x400100]
symbol = "__vt__3Foo"
toc_section = 1
toc_offset = 0x20
mac_section = 1
mac_offset = 0x100
mac_size = 0x14
sha256 = "{digest(data[0x100:0x114])}"
rtti_section = 1
rtti_offset = 0x80
rtti_size = 0xc
rtti_sha256 = "{digest(data[0x80:0x8c])}"
evidence = "fixture with three loader-linked slots"
[[vtables.declarations]]
source = "include/foo.h"
class = "Foo"
base = "Base"
methods = ["virtual int open(int)", "virtual void close()", "virtual int main(int&)"]
''' + "\n".join(slots))
        pef = Image(bytes(code), bytes(data))
        loader = Loader(pointers)
        name = "__vt__3Foo"
        code_hunk = SimpleNamespace(xrefs=((0, "HUNK_XREF_16BIT_IL", name),))
        cell = SimpleNamespace(name=name, storage_class="TC", data=bytes(4),
                               xrefs=((0, "HUNK_XREF_32BIT", name),))
        return source, pef, loader, code_hunk, cell

    def test_complete_external_binding_and_fail_closed_inputs(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source, pef, loader, code, cell = self.fixture(root)
            args = dict(unit="foo", retail_va=0x400100)
            self.assertEqual(external_bindings(root, pef, loader, code, (cell,), **args),
                             {"__vt__3Foo": Address(1, 0x100)})
            loader.pointers[Address(1, 0x10c)] = Address(1, 0x220)
            with self.assertRaises(ObjectError):
                external_bindings(root, pef, loader, code, (cell,), **args)
            loader.pointers[Address(1, 0x10c)] = Address(1, 0x210)
            emitted = SimpleNamespace(name=cell.name, storage_class="RW",
                                      data=bytes(20), xrefs=())
            with self.assertRaises(ObjectError):  # a same-TU table needs the reviewed slot layout
                external_bindings(root, pef, loader, code, (cell, emitted), **args)
            defined = SimpleNamespace(name=cell.name, storage_class="RW", data=bytes(20), xrefs=(
                (0, "HUNK_XREF_32BIT", "__RTTI__3Foo"), (8, "HUNK_XREF_32BIT", "open__3FooFi"),
                (12, "HUNK_XREF_32BIT", "close__3FooFv"), (16, "HUNK_XREF_32BIT", "main__3FooFRi")))
            self.assertEqual(external_bindings(root, pef, loader, code, (cell, defined), **args),
                             {"__vt__3Foo": Address(1, 0x100)})
            source.write_text("class Foo : public Base { virtual int open(int); };\n")
            with self.assertRaises(ObjectError):
                external_bindings(root, pef, loader, code, (cell,), **args)


if __name__ == "__main__":
    unittest.main()
