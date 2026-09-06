"""Declarator and field-layout controls for the NB11 reference exporter."""
import struct
import unittest

from homm3.core.nb11_types import Types
from homm3.core.nb11 import Procedure, Variable


def record(leaf, body):
    return struct.pack("<HH", len(body) + 2, leaf) + body


def name(value):
    raw = value.encode("latin1")
    return bytes([len(raw)]) + raw


class TypesTest(unittest.TestCase):
    def setUp(self):
        self.records = {
            0x1000: record(0x1001, struct.pack("<IH", 0x74, 1)),
            0x1001: record(0x1002, struct.pack("<II", 0x1000, 10)),
            0x1002: record(0x1002, struct.pack("<II", 0x74, 10 | 0x400)),
            0x1003: record(0x1002, struct.pack("<II", 0x74, 10 | 0x20)),
            0x1004: record(0x1003, struct.pack("<IIH", 0x74, 0x74, 12) + name("")),
            0x1005: record(0x1201, struct.pack("<II", 1, 0x74)),
            0x1006: record(0x1008, struct.pack("<IBBHI", 3, 0, 0, 1, 0x1005)),
            0x1007: record(0x1002, struct.pack("<II", 0x1006, 10)),
            0x1008: record(0x1004, struct.pack("<HHIIIH", 0, 0, 0, 0, 0, 4) + name("Widget")),
            0x1009: record(0x1001, struct.pack("<IH", 0x1008, 3)),
            0x100a: record(0x1002, struct.pack("<II", 0x1009, 10 | 0x400)),
            0x100b: record(0x1009, struct.pack("<IIIBBHIi", 0x74, 0x1008, 0x100a,
                                             16, 0, 1, 0x1005, -4)),
        }
        self.types = Types(self.records)

    def test_pointer_and_pointee_const_and_reference_are_distinct(self):
        self.assertEqual(self.types.declaration(0x1001, "p"), "const int *p")
        self.assertEqual(self.types.declaration(0x1002, "p"), "int * const p")
        self.assertEqual(self.types.declaration(0x1003, "p"), "int &p")

    def test_array_extent_and_function_pointer_precedence(self):
        self.assertEqual(self.types.declaration(0x1004, "a"), "int a[3]")
        self.assertEqual(self.types.declaration(0x1007, "callback"), "void (__cdecl *callback)(int)")

    def test_member_signature_keeps_this_qualifiers_and_adjustment(self):
        self.assertEqual(self.types.function(0x100b, "Widget::read", ["value"]),
                         "int __shcall Widget::read(int value) const volatile")
        self.assertEqual(self.types.get(0x100b)["this_adjust"], -4)

    def test_special_members_do_not_acquire_a_return_type(self):
        types = Types({**self.records,
                       0x1010: record(0x1002, struct.pack("<II", 0x1008, 10)),
                       0x1011: record(0x1201, struct.pack("<I", 0)),
                       0x1012: record(0x1009, struct.pack("<IIIBBHIi", 3, 0x1008, 0x1010,
                                                        16, 0, 0, 0x1011, 0))})
        self.assertEqual(types.function(0x1012, "Widget::Widget"), "__shcall Widget::Widget()")
        self.assertEqual(types.function(0x1012, "Widget::~Widget"), "__shcall Widget::~Widget()")

    def test_fields_preserve_signed_enums_offsets_access_and_virtual_slots(self):
        member = struct.pack("<HHIH", 0x1405, 2, 0x74, 12) + name("field")
        enumerator = struct.pack("<HHHh", 0x403, 3, 0x8001, -2) + name("negative")
        method = struct.pack("<HHIi", 0x140b, 3 | (4 << 2), 0x100b, 16) + name("read")
        types = Types({**self.records, 0x1010: record(0x1203, member + enumerator + method)})
        fields = types.fields(0x1010)
        self.assertEqual((fields[0]["offset"], fields[0]["access"]), (12, "protected"))
        self.assertEqual(fields[1]["value"], -2)
        self.assertEqual((fields[2]["property"], fields[2]["vtable_offset"]), ("introducing virtual", 16))

    def test_truncated_class_name_requires_independent_unambiguous_udt(self):
        raw = record(0x1004, struct.pack("<HHIIIHI", 1, 0, 0, 0, 0, 0x8004, 200000) + b"\x04g")
        recovered = Types({0x1010: raw}, {0x1010: {"game"}}).get(0x1010)
        self.assertEqual((recovered["name"], recovered["name_source"]), ("game", "S_UDT"))
        self.assertTrue(recovered["name_truncated"])
        unresolved = Types({0x1010: raw}).get(0x1010)
        self.assertEqual(unresolved["name"], "cv_type_1010")

    def test_recursive_or_missing_type_keeps_its_id(self):
        self.assertEqual(self.types.declaration(0x9999, "x"), "cv_type_9999 x")
        types = Types({0x1010: record(0x1002, struct.pack("<II", 0x1010, 10))})
        self.assertEqual(types.declaration(0x1010, "p"), "cv_type_1010 *p")

    def test_hidden_return_storage_does_not_shift_source_argument_names(self):
        proc = Procedure("Widget::read", 32, type_index=0x100b, variables=[
            Variable("this", 0x100a, "param", "sp+0x0", None, 1),
            Variable("__$ReturnUdt", 0, "param", "sp+0x4", None, 2),
            Variable("value", 0x74, "param", "sp+0x8", None, 3)])
        self.assertEqual(self.types.procedure_signature(proc),
                         "int __shcall Widget::read(int value) const volatile")
        proc.variables.pop()
        self.assertEqual(self.types.procedure_signature(proc),
                         "int __shcall Widget::read(int) const volatile")

    def test_missing_function_type_still_preserves_known_arguments(self):
        proc = Procedure("unknown", 32, variables=[
            Variable("value", 0x74, "param", "sp+0x0", None, 1)])
        self.assertEqual(self.types.procedure_signature(proc),
                         "/* return type / calling convention unavailable */ unknown(int value)")


if __name__ == "__main__":
    unittest.main()
