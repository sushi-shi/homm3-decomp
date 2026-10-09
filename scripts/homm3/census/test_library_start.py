"""A library member's own function inside the project's code is no name."""
import unittest

from homm3.census import libraries


class LibraryStartTest(unittest.TestCase):
    def test_the_first_run_of_member_functions_opens_the_library_code(self):
        order = [0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70]
        once = {0x20, 0x50, 0x60, 0x70}
        self.assertEqual(libraries.library_start(order, once), 0x50)

    def test_no_run_no_start(self):
        self.assertIsNone(libraries.library_start([0x10, 0x20], {0x10, 0x20}))


if __name__ == "__main__":
    unittest.main()
