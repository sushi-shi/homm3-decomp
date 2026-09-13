"""Check the current filter builder against an independent UI record oracle."""
import importlib.util
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[3]


class FilterWidgetRecoveryTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('g++'), 'requires native C++ compiler')
    def test_current_widgets_fields_order_and_array_identity(self):
        spec = importlib.util.spec_from_file_location(
            'filter_family', ROOT / 'scripts/experiments/generate-filter-widget-recovery-family.py')
        family = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(family)
        actual = family.original_body()
        variants = [('Actual', actual, True)]
        variants += [
            ('WrongHighlight',actual.replace('m_highlightedFrame = 2','m_highlightedFrame = 3'),False),
            ('WrongDisabled',actual.replace('setDisabledFrame(1)','setDisabledFrame(0)'),False),
            ('WrongDefault',actual.replace('m_randomMapOptions[1] = 2','m_randomMapOptions[1] = 3'),False),
            ('MissingWidget', re.sub(
                r'^        (?:m_widgets|widgets)\.(?:push_back|insert)\([^\n]+\);\n',
                '', actual, count=1, flags=re.M), False),
            ('WrongWidgetId',actual.replace('0x119, "RanSizS.def"','0x11a, "RanSizS.def"'),False),
        ]
        fixture = (ROOT / 'scripts/experiments/filter-widget-recovery-oracle.cpp').read_text()
        setter = next(line.strip() for line in (ROOT / 'include/button.h').read_text().splitlines()
                      if 'void setDisabledFrame(long frame)' in line)
        fixture = fixture.replace('// @DISABLED_SETTER@', setter)
        programs, checks = [], []
        for name, body, valid in variants:
            if not valid:
                self.assertNotEqual(body, actual, name)
            programs.append('namespace ' + name + ' {\n' + fixture.replace('// @BODY@', body) + '\n}')
            checks.append('if (' + name + '::check() != ' + str(valid).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = '#include <vector>\n#include <string>\n#include <cstdio>\n' + '\n'.join(programs)
        program += '\nint main() {\n' + '\n'.join(checks) + '\n}\n'
        with tempfile.TemporaryDirectory(prefix='homm3-filter-recovery-') as directory:
            cpp, executable = Path(directory) / 'oracle.cpp', Path(directory) / 'oracle'
            cpp.write_text(program)
            result = subprocess.run(['g++', '-std=c++98', '-O1', str(cpp), '-o', str(executable)],
                                    capture_output=True, text=True, timeout=180)
            self.assertEqual(result.returncode, 0, result.stderr[-6000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
