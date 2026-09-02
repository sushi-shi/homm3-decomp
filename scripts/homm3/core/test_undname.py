"""Tests for the qualified-name reduction of demangled MSVC names."""
from __future__ import annotations

import unittest

from homm3.core import undname


class QualifiedNameTest(unittest.TestCase):
    def test_member_free_special_and_data_names(self):
        cases = {
            "public: int __thiscall game::GetTeam(int) const": "game::GetTeam",
            "public: virtual __thiscall CHeroWindowEx::~CHeroWindowEx(void)":
                "CHeroWindowEx::~CHeroWindowEx",
            "public: virtual void * __thiscall type_transformer_slot::"
            "`scalar deleting dtor'(unsigned int)":
                "type_transformer_slot::`scalar deleting dtor'",
            "public: class S & __thiscall S::operator=(class S const &)":
                "S::operator=",
            "public: bool __thiscall type_point::operator==(struct type_point "
            "const &) const": "type_point::operator==",
            "public: int __thiscall TFoo::operator()(void)": "TFoo::operator()",
            "void * __cdecl operator new(unsigned int)": "operator new",
            "long __fastcall AI_value_of_event(class hero const *, struct "
            "type_point, long &)": "AI_value_of_event",
            "class TTextResource *gpGeneralText": "gpGeneralText",
            "public: __thiscall std::map<struct K, class V *, struct "
            "std::less<struct K>>::~map<struct K, class V *, struct "
            "std::less<struct K>>(void)":
                "std::map<struct K, class V *, struct std::less<struct K>>::"
                "~map<struct K, class V *, struct std::less<struct K>>",
        }
        for demangled, expected in cases.items():
            self.assertEqual(undname.qualified(demangled), expected, demangled)

    def test_signature_strip_and_bare(self):
        self.assertEqual(undname.strip_signature("army::GetName() const"),
                         "army::GetName")
        self.assertEqual(undname.strip_signature("f(int (*)(int))"), "f")
        self.assertEqual(undname.bare("game::GetTeam"), "GetTeam")
        self.assertEqual(undname.bare("std::map<a::b, c>::~map<a::b, c>"),
                         "~map<a::b, c>")
        self.assertEqual(undname.bare("free_fn"), "free_fn")

    @unittest.skipUnless(undname.available(), "llvm-undname not on PATH")
    def test_batch_demangle_pairs_results_and_skips_failures(self):
        result = undname.qualified_names(
            ["?GetTeam@game@@QBEHH@Z", "not_mangled", "?$E463",
             "??1CHeroWindowEx@@UAE@XZ"])
        self.assertEqual(result, {
            "?GetTeam@game@@QBEHH@Z": "game::GetTeam",
            "??1CHeroWindowEx@@UAE@XZ": "CHeroWindowEx::~CHeroWindowEx"})


if __name__ == "__main__":
    unittest.main()
