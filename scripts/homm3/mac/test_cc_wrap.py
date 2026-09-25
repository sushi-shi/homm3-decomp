"""Full-TU CodeWarrior diagnostics are summarized from their real messages."""
import unittest

from homm3.mac import cc_wrap


class TestFirstError(unittest.TestCase):
    def test_reports_file_line_and_message_after_the_caret(self):
        log = ("wine MWCPPC.exe -o build/mac/obj/town.o src/town.cpp\n"
               "### MWCPPC.exe Compiler:\n"
               "#    In: include\\town.h\n"
               "#     222: extern long g_x;\n"
               "# Warning:        ^^^^^^^\n"
               "#   implicit 'int' is no longer supported in C++\n"
               "### MWCPPC.exe Compiler:\n"
               "#    In: include\\town.h\n"
               "#     337: badtype m_built;\n"
               "#   Error: ^^^^^^^\n"
               "#   declaration syntax error\n")
        self.assertEqual(cc_wrap.first_error(log),
                         "include/town.h:337: declaration syntax error")

    def test_preprocessor_error_is_reported(self):
        log = ("wine MWCPPC.exe\n### MWCPPC.exe Compiler:\n#    File: direct.h\n"
               "#      22:  #error ERROR: Only Mac or Win32 targets supported!\n")
        self.assertIn("Only Mac or Win32", cc_wrap.first_error(log))


if __name__ == "__main__":
    unittest.main()
