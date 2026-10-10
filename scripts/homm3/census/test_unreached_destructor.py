"""Below the library band a library deleting destructor names an address
only when the image's RTTI names its class's vtable (h3maped 0x43fa68 is
TMine's, not std::basic_iostream's)."""
import unittest

from homm3.census import libraries

IOSTREAM = "??_G?$basic_iostream@DU?$char_traits@D@std@@@std@@UAEPAXI@Z"


class UnreachedDestructorTest(unittest.TestCase):
    def test_a_class_without_an_rtti_vtable_is_unreached(self):
        self.assertTrue(libraries.unreached_destructor(IOSTREAM, {"istrstream@std": 0x13f9f4}))
        self.assertTrue(libraries.unreached_destructor("??_Gstrstream@std@@UAEPAXI@Z", {}))

    def test_a_class_with_an_rtti_vtable_is_reached(self):
        vtables = {"?$basic_iostream@DU?$char_traits@D@std@@@std": 0x1000}
        self.assertFalse(libraries.unreached_destructor(IOSTREAM, vtables))

    def test_other_members_are_never_unreached(self):
        self.assertFalse(libraries.unreached_destructor("??1strstream@std@@UAE@XZ", {}))


if __name__ == "__main__":
    unittest.main()
